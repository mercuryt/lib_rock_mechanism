#pragma once
#include "types.h"
#include "json.h"
#include "index.h"
#include "dataVector.h"
#include <cassert>
#include <compare>
#include <functional>
#include <variant>
template<typename Index, typename ReferenceIndex>
class ReferenceData;
template<typename Index, typename ReferenceIndex>
class Reference
{
	using Data = ReferenceData<Index, ReferenceIndex>;
	ReferenceIndex m_referenceIndex;
public:
	Reference(const ReferenceIndex& index) : m_referenceIndex(index) { }
	Reference(const Reference& other) : m_referenceIndex(other.m_referenceIndex) { }
	Reference(Reference&& other) noexcept : m_referenceIndex(other.m_referenceIndex) { }
	Reference() = default;
	Reference(const Data& dataStore, Index index) { setIndex(index, dataStore); }
	Reference& operator=(const Reference& other) { m_referenceIndex = other.m_referenceIndex; return *this; }
	void setReferenceIndex(const ReferenceIndex& index) { m_referenceIndex = index; }
	void setIndex(const Index& index, const Data& referenceData) { m_referenceIndex = referenceData.getReferenceIndex(index); }
	void clear() { m_referenceIndex.clear(); }
	void load(const Json& data) { m_referenceIndex = data.get<ReferenceIndex>(); }
	void validate(const Data& dataStore) { dataStore.validateReference(m_referenceIndex); }
	[[nodiscard]] Index getIndex(const Data& referenceData) const { assert(exists()); return referenceData.getIndex(m_referenceIndex); }
	[[nodiscard]] ReferenceIndex getReferenceIndex() const { return m_referenceIndex; }
	[[nodiscard]] bool exists() const { return m_referenceIndex.exists(); }
	[[nodiscard]] bool empty() const { return m_referenceIndex.empty(); }
	[[nodiscard]] bool operator==(const Reference& other) const { return other.m_referenceIndex == m_referenceIndex; } 
	[[nodiscard]] std::strong_ordering operator<=>(const Reference& other) const { return m_referenceIndex <=> other.m_referenceIndex; }
	[[nodiscard]] size_t hash() const { return m_referenceIndex.get(); }
	struct Hash { [[nodiscard]] size_t operator()(const Reference<Index, ReferenceIndex>& reference) const { return reference.get(); } };
};
using ItemReference = Reference<ItemIndex, ItemReferenceIndex>;
inline void to_json(Json& data, const ItemReference& ref) { data = ref.getReferenceIndex(); }
inline void from_json(const Json& data, ItemReference& ref) { ref.setReferenceIndex(data.get<ItemReferenceIndex>()); }
using ActorReference = Reference<ActorIndex, ActorReferenceIndex>;
inline void to_json(Json& data, const ActorReference& ref) { data = ref.getReferenceIndex(); }
inline void from_json(const Json& data, ActorReference& ref) { ref.setReferenceIndex(data.get<ActorReferenceIndex>()); }
/*
	ReferenceData correlates Indexes to ReferenceIndexes for Actors and Items.
	ReferenceIndices are generated from the length of the m_indicesByReference vector.
	ReferenceIndices area recycled via the m_unusedReferenceIndex set.
*/
template<typename Index, typename ReferenceIndex>
class ReferenceData
{
	DataVector<Index, ReferenceIndex> m_indicesByReference;
	DataVector<ReferenceIndex, Index> m_referencesByIndex;
	SmallSet<ReferenceIndex> m_unusedReferenceIndices;
	[[nodiscard]] ReferenceIndex getReferenceIndex(const Index &actor) const { return m_referencesByIndex[actor]; }
	[[nodiscard]] Index getIndex(const ReferenceIndex& reference) const { return m_indicesByReference[reference]; }
	friend class Reference<Index, ReferenceIndex>;
public:
	void load(const Json& data)
	{
		data["index"].get_to(m_indicesByReference);
		data["reference"].get_to(m_referencesByIndex);
		data["unused"].get_to(m_unusedReferenceIndices);
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
	}
	void remove(const Index& index)
	{
		assert(m_referencesByIndex[index].exists());
		ReferenceIndex refIndex = m_referencesByIndex[index];
		assert(m_indicesByReference[refIndex].exists());
		assert(m_indicesByReference[refIndex] == index);
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
	}
	void reserve(const Index& size)
	{
		reserve(size.get());
	}
	void validateReference(const ReferenceIndex& ref) const
	{
		assert(ref < m_indicesByReference.size());
		assert(m_indicesByReference[ref].exists());
	}
	[[nodiscard]] size_t size() const { return m_referencesByIndex.size(); }
	[[nodiscard]] auto getReference(const Index& index) const -> Reference<Index, ReferenceIndex> const { return m_referencesByIndex[index]; }
	[[nodiscard]] Json toJson() const
	{
		return {{"index", m_indicesByReference}, {"reference", m_referencesByIndex}, {"unused", m_unusedReferenceIndices}};
	}
	// For testing.
	[[nodiscard]] const auto& getIndices() const { return m_indicesByReference; }
	[[nodiscard]] const DataVector<ReferenceIndex, Index>& getReferenceIndices() const { return m_referencesByIndex; }
	[[nodiscard]] const auto& getUnusedReferenceIndices() const { return m_unusedReferenceIndices; }
};
using ItemReferenceData = ReferenceData<ItemIndex, ItemReferenceIndex>;
inline void to_json(Json& data, const ItemReferenceData& refData) { data = refData.toJson(); }
inline void from_json(const Json& data, ItemReferenceData& refData) { refData.load(data); }
using ActorReferenceData = ReferenceData<ActorIndex, ActorReferenceIndex>;
inline void to_json(Json& data, const ActorReferenceData& refData) { data = refData.toJson(); }
inline void from_json(const Json& data, ActorReferenceData& refData) { refData.load(data); }
// Polymorphic reference wrapes either an actor or an item.
#include "actorOrItemIndex.h"
class ActorOrItemReference
{
	std::variant<std::monostate, ActorReference, ItemReference> m_reference;
public:
	void setActor(const ActorReference& target) { m_reference.emplace<1>(target); }
	void setItem(const ItemReference& target) { m_reference.emplace<2>(target); }
	void clear() { m_reference.emplace<0>(); }
	void load(const Json& data)
	{
		if(data[0])
		{
			ActorReference actor = data[1].get<ActorReference>();
			setActor(actor);
		}
		else
		{
			ItemReference item = data[1].get<ItemReference>();
			setItem(item);
		}
	}
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
			HasShapeIndex::cast(std::get<1>(m_reference).getIndex(actorData)) :
			HasShapeIndex::cast(std::get<2>(m_reference).getIndex(itemData));
	}
	[[nodiscard]] ActorOrItemIndex getIndexPolymorphic(const ActorReferenceData& actorData, const ItemReferenceData& itemData) const
	{
		if(isActor())
			return ActorOrItemIndex::createForActor(ActorIndex::cast(getIndex(actorData, itemData)));
		else
			return ActorOrItemIndex::createForItem(ItemIndex::cast(getIndex(actorData, itemData)));
	}
	[[nodiscard]] static ActorOrItemReference createForActor(const ActorReference& target) {  ActorOrItemReference output; output.setActor(target); return output;}
	[[nodiscard]] static ActorOrItemReference createForItem(const ItemReference& target) {  ActorOrItemReference output; output.setItem(target); return output;}
	[[nodiscard]] Json toJson() const
	{
		if(isActor())
			return Json{true, std::get<1>(m_reference)};
		else
			return Json{false, std::get<2>(m_reference)};
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
					assert(false);
			}
		}
	};
};
inline void to_json(Json& data, const ActorOrItemReference& ref) { data = ref.toJson(); }
inline void from_json(const Json& data, ActorOrItemReference& ref) { ref.load(data); }