#pragma once
#include "numericTypes/types.h"
#include "json.h"
#include "numericTypes/index.h"
#include "dataStructures/strongVector.h"
#include <cassert>
#include <compare>
#include <functional>
#include <variant>
#ifndef NDEBUG
	//#define TRACK_REFERENCES;
#endif

template<typename Index, typename ReferenceIndex>
class ReferenceData;
template<typename Index, typename ReferenceIndex>
class Reference
{
	using Data = ReferenceData<Index, ReferenceIndex>;
	ReferenceIndex m_referenceIndex;
	#ifdef TRACK_REFERENCES
		Data* m_data = nullptr;
		std::shared_ptr<bool> m_destruct;
	#endif
public:
	Reference() = default;
	Reference(const ReferenceIndex& index,[[maybe_unused]] Data& data) :
		m_referenceIndex(index)
	{
		#ifdef TRACK_REFERENCES
			if(m_referenceIndex.exists())
			{
				setReferenceData(data);
				m_data->recordReference(*this);
			}
		#endif
	}
	Reference(const Reference<Index, ReferenceIndex>& other)
	{
		m_referenceIndex = other.m_referenceIndex;
		#ifdef TRACK_REFERENCES
			if(m_referenceIndex.exists())
			{
				// Discard const from other.m_data for reference counting.
				setReferenceData(const_cast<ReferenceData<Index, ReferenceIndex>&>(*other.m_data));
				m_data->recordReference(*this);
			}
		#endif
	}
	Reference(Reference<Index, ReferenceIndex>&& other) noexcept
	{
		m_referenceIndex = other.m_referenceIndex;
		#ifdef TRACK_REFERENCES
			if(m_referenceIndex.exists())
			{
				setReferenceData(*other.m_data);
				other.clear();
				m_data->recordReference(*this);
			}
		#endif
	}
	Reference(const Json& data, Data& dataStore)
	{
		load(data, dataStore);
	}
	~Reference()
	{
		#ifdef TRACK_REFERENCES
			if(m_referenceIndex.exists() && !*m_destruct)
				setReferenceIndex(ReferenceIndex::null());
		#endif
	}
	Reference& operator=(const Reference& other)
	{
		// Discard const from other.m_data for reference counting.
		#ifdef TRACK_REFERENCES
			maybeSetReferenceData(const_cast<ReferenceData<Index, ReferenceIndex>&>(*other.m_data));
		#endif
		setReferenceIndex(other.m_referenceIndex);
		return *this;
	}
	Reference& operator=(Reference&& other) noexcept
	{
		#ifdef TRACK_REFERENCES
			if(m_referenceIndex.exists())
				m_data->removeReference(*this);
		#endif
		m_referenceIndex = other.m_referenceIndex;
		#ifdef TRACK_REFERENCES
			if(m_referenceIndex.exists())
			{
				maybeSetReferenceData(*other.m_data);
				other.clear();
				m_data->recordReference(*this);
			}
		#endif
		return *this;
	}
	#ifdef TRACK_REFERENCES
	void setReferenceData(Data& data)
	{
		assert(m_data == nullptr);
		assert(data.m_destruct != nullptr);
		assert(m_destruct == nullptr);
		m_data = &data;
		m_destruct = m_data->m_destruct;
	}
	void maybeSetReferenceData(Data& data)
	{
			if(m_data == nullptr) setReferenceData(data);
			else assert(m_data == &data);
	}
	#endif
	void setReferenceIndex(const ReferenceIndex& ref)
	{
		#ifdef TRACK_REFERENCES
			assert(m_data != nullptr);
			if(m_referenceIndex.exists()) m_data->removeReference(*this);
		#endif
		m_referenceIndex = ref;
		#ifdef TRACK_REFERENCES
			if(m_referenceIndex.exists()) m_data->recordReference(*this);
		#endif
	}
	void setIndex(const Index& index, Data& dataStore)
	{
		#ifdef TRACK_REFERENCES
			maybeSetReferenceData(dataStore);
		#endif
		setReferenceIndex(dataStore.getReferenceIndex(index));
	}
	void clear() { setReferenceIndex(ReferenceIndex::null()); }
	void load(const Json& data, [[maybe_unused]] Data& dataStore)
	{
		#ifdef TRACK_REFERENCES
			setReferenceData(dataStore);
		#endif
		setReferenceIndex(data.get<ReferenceIndex>());
	}
	void validate(const Data& dataStore) const
	{
		#ifdef TRACK_REFERENCES
			assert(m_data == &dataStore);
		#endif
		dataStore.validateReference(m_referenceIndex);
	}
	[[nodiscard]] Index getIndex(const Data& dataStore) const
	{
		assert(exists());
		#ifdef TRACK_REFERENCES
			assert(&dataStore == m_data);
		#endif
		return dataStore.getIndex(m_referenceIndex);
	}
	[[nodiscard]] ReferenceIndex getReferenceIndex() const { return m_referenceIndex; }
	[[nodiscard]] bool exists() const { return m_referenceIndex.exists(); }
	[[nodiscard]] bool empty() const { return m_referenceIndex.empty(); }
	[[nodiscard]] bool operator==(const Reference& other) const
	{
		#ifdef TRACK_REFERENCES
			assert(m_data == other.m_data);
		#endif
		return other.m_referenceIndex == m_referenceIndex;
	}
	[[nodiscard]] std::strong_ordering operator<=>(const Reference& other) const
	{
		#ifdef TRACK_REFERENCES
			assert(m_data == other.m_data);
		#endif
		return m_referenceIndex <=> other.m_referenceIndex;
	}
	[[nodiscard]] size_t hash() const { return m_referenceIndex.get(); }
	[[nodiscard]] Json toJson() const { return m_referenceIndex.get(); }
	struct Hash { [[nodiscard]] size_t operator()(const Reference<Index, ReferenceIndex>& reference) const { return reference.get(); } };
};
template<typename Index, typename ReferenceIndex>
void to_json(Json& data, const Reference<Index, ReferenceIndex>& ref) { data = ref.toJson(); }
using ItemReference = Reference<ItemIndex, ItemReferenceIndex>;
using ActorReference = Reference<ActorIndex, ActorReferenceIndex>;
/*
	ReferenceData correlates Indexes to ReferenceIndexes for Actors and Items.
	ReferenceIndices are generated from the length of the m_indicesByReference vector.
	ReferenceIndices area recycled via the m_unusedReferenceIndex set.
*/
template<typename Index, typename ReferenceIndex>
class ReferenceData
{
	StrongVector<Index, ReferenceIndex> m_indicesByReference;
	StrongVector<ReferenceIndex, Index> m_referencesByIndex;
	SmallSet<ReferenceIndex> m_unusedReferenceIndices;
	#ifdef TRACK_REFERENCES
		StrongVector<SmallSet<Reference<Index, ReferenceIndex>*>, ReferenceIndex> m_references;
		std::shared_ptr<bool> m_destruct;
public: ~ReferenceData() { (*m_destruct) = true; }
		ReferenceData() { m_destruct = std::make_shared<bool>(false); }
		void recordReference(Reference<Index, ReferenceIndex>& ref)
		{
			for(auto& references : m_references)
				assert(!references.contains(&ref));
			m_references[ref.getReferenceIndex()].insert(&ref);
		}
		void removeReference(Reference<Index, ReferenceIndex>& ref)
		{
			for(auto& references : m_references)
				if(&references != &m_references[ref.getReferenceIndex()])
					assert(!references.contains(&ref));
			m_references[ref.getReferenceIndex()].erase(&ref);
		}
	#endif
private:
	[[nodiscard]] ReferenceIndex getReferenceIndex(const Index &actor) const { return m_referencesByIndex[actor]; }
	[[nodiscard]] Index getIndex(const ReferenceIndex& reference) const { assert(m_indicesByReference[reference].exists()); return m_indicesByReference[reference]; }
	friend class Reference<Index, ReferenceIndex>;
public:
	void load(const Json& data)
	{
		data["m_indicesByReference"].get_to(m_indicesByReference);
		data["m_referencesByIndex"].get_to(m_referencesByIndex);
		data["m_unusedReferenceIndices"].get_to(m_unusedReferenceIndices);
		#ifdef TRACK_REFERENCES
			m_references.resize(m_referencesByIndex.size());
		#endif
	}
	void add(const Index& index)
	{
		assert(index == m_referencesByIndex.size());
		assert(!m_indicesByReference.contains(index));
		ReferenceIndex refIndex;
		if(m_unusedReferenceIndices.empty())
		{
			refIndex = ReferenceIndex::create(m_indicesByReference.size());
			m_indicesByReference.resize(refIndex + 1);
		}
		else
		{
			refIndex = m_unusedReferenceIndices.back();
			m_unusedReferenceIndices.popBack();
		}
		assert(refIndex.exists());
		assert(!m_referencesByIndex.contains(refIndex));
		m_indicesByReference[refIndex] = index;
		m_referencesByIndex.add(refIndex);
		#ifdef TRACK_REFERENCES
			if(m_references.size() < m_indicesByReference.size())
				m_references.resize(m_indicesByReference.size());
		#endif
	}
	void remove(const Index& index)
	{
		assert(m_referencesByIndex[index].exists());
		ReferenceIndex refIndex = m_referencesByIndex[index];
		assert(m_indicesByReference[refIndex].exists());
		assert(m_indicesByReference[refIndex] == index);
		#ifdef TRACK_REFERENCES
			assert(m_references[refIndex].empty());
		#endif
		m_referencesByIndex.remove(index);
		// Index was not the last item, a still valid item has been moved and it's reference must be updated.
		if(m_referencesByIndex.size() > index)
		{
			ReferenceIndex refNowOccupiningOldSlot = m_referencesByIndex[index];
			assert(m_indicesByReference[refNowOccupiningOldSlot] != index);
			m_indicesByReference[refNowOccupiningOldSlot] = index;
		}
		// Reference indices must remain stable, remove only from the end, otherwise clear and record unused.
		if(refIndex == m_indicesByReference.size() - 1)
		{
			m_indicesByReference.popBack();
			// After removing from end, check if newly exposed end is unused, and remove it as well.
			while(!m_indicesByReference.empty() && m_indicesByReference.back().empty())
			{
				m_unusedReferenceIndices.erase(ReferenceIndex::create(m_indicesByReference.size() - 1));
				m_indicesByReference.popBack();
			}
		}
		else
		{
			m_indicesByReference[refIndex].clear();
			m_unusedReferenceIndices.insert(refIndex);
		}
	}
	void reserve(uint size)
	{
		m_referencesByIndex.reserve(size);
		// The current 'next reference' is determined by the length of indices by reference, which is why we reserve rather then resize here.
		m_indicesByReference.reserve(size);
		#ifdef TRACK_REFERENCES
			m_references.reserve(size);
		#endif
	}
	void reserve(const Index& size)
	{
		reserve(size.get());
	}
	void validateReference([[maybe_unused]] const ReferenceIndex& ref) const
	{
		assert(ref < m_indicesByReference.size());
		assert(m_indicesByReference[ref].exists());
	}
	[[nodiscard]] size_t size() const { return m_referencesByIndex.size(); }
	[[nodiscard]] auto getReference(const Index& index) -> const Reference<Index, ReferenceIndex> { return Reference<Index, ReferenceIndex>(m_referencesByIndex[index], *this); }
	// For testing.
	[[nodiscard]] const auto& getIndices() const { return m_indicesByReference; }
	[[nodiscard]] const StrongVector<ReferenceIndex, Index>& getReferenceIndices() const { return m_referencesByIndex; }
	[[nodiscard]] const auto& getUnusedReferenceIndices() const { return m_unusedReferenceIndices; }
	#ifdef TRACK_REFERENCES
	[[nodiscard]] bool hasReferencesTo(const Index& index) const { return !m_references[m_referencesByIndex[index]].empty(); }
	#endif
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(ReferenceData, m_indicesByReference, m_referencesByIndex, m_unusedReferenceIndices);
};
template<typename Index, typename ReferenceIndex>
inline void from_json(const Json& data, ReferenceData<Index, ReferenceIndex>& ref) { ref.load(data); }
using ItemReferenceData = ReferenceData<ItemIndex, ItemReferenceIndex>;
using ActorReferenceData = ReferenceData<ActorIndex, ActorReferenceIndex>;
// Polymorphic reference wrapes either an actor or an item.
#include "actorOrItemIndex.h"
class ActorOrItemReference
{
	std::variant<std::monostate, ActorReference, ItemReference> m_reference;
public:
	ActorOrItemReference() = default;
	ActorOrItemReference(const Json& data, Area& area) { load(data, area); }
	void setActor(const ActorReference& target) { m_reference.emplace<1>(target); }
	void setItem(const ItemReference& target) { m_reference.emplace<2>(target); }
	void clear() { m_reference.emplace<0>(); }
	void load(const Json& data, Area& area);
	[[nodiscard]] bool isActor() const
	{
		assert(exists());
		return m_reference.index() == 1;
	}
	[[nodiscard]] bool isItem() const
	{
		return !isActor();
	}
	[[nodiscard]] bool exists() const { return m_reference.index() != 0; }
	[[nodiscard]] bool operator==(const ActorOrItemReference& ref) const { return ref.m_reference == m_reference; }
	[[nodiscard]] HasShapeIndex getIndex(const ActorReferenceData& actorData, const ItemReferenceData& itemData) const
	{
		assert(exists());
		return isActor() ?
			std::get<1>(m_reference).getIndex(actorData).toHasShape() :
			std::get<2>(m_reference).getIndex(itemData).toHasShape();
	}
	[[nodiscard]] ActorOrItemIndex getIndexPolymorphic(const ActorReferenceData& actorData, const ItemReferenceData& itemData) const
	{
		if(isActor())
			return ActorOrItemIndex::createForActor(std::get<1>(m_reference).getIndex(actorData));
		else
			return ActorOrItemIndex::createForItem(std::get<2>(m_reference).getIndex(itemData));
	}
	[[nodiscard]] ActorReference toActorReference() const { return std::get<1>(m_reference); }
	[[nodiscard]] ItemReference toItemReference() const { return std::get<2>(m_reference); }
	[[nodiscard]] ActorIndex toActorIndex(const ActorReferenceData& data) const { return std::get<1>(m_reference).getIndex(data); }
	[[nodiscard]] ItemIndex toItemIndex(const ItemReferenceData& data) const { return std::get<2>(m_reference).getIndex(data); }
	[[nodiscard]] static ActorOrItemReference createForActor(const ActorReference& target) {  ActorOrItemReference output; output.setActor(target); return output;}
	[[nodiscard]] static ActorOrItemReference createForItem(const ItemReference& target) {  ActorOrItemReference output; output.setItem(target); return output;}
	[[nodiscard]] Json toJson() const
	{
		if(isActor())
			return {true, std::get<1>(m_reference).toJson()};
		else
			return {false, std::get<2>(m_reference).toJson()};
	}
	struct Hash
	{
		[[nodiscard]]size_t operator()(const ActorOrItemReference& reference) const
		{
			switch(reference.m_reference.index())
			{
				case 0:
					return 0;
				case 1:
					return std::get<1>(reference.m_reference).hash();
				case 2:
					return std::get<2>(reference.m_reference).hash();
				default:
					std::unreachable();
			}
		}
	};
};
inline void to_json(Json& data, const ActorOrItemReference& ref) { data = ref.toJson(); }