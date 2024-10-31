#pragma once
#include "types.h"
#include "json.h"
#include "index.h"
#include <cassert>
#include <compare>
#include <functional>
#include <variant>
class Area;
// Each Item holds a reference target in a unique_ptr.
// Code outside the engine must use an itemReference rather then storing ItemIndex.
// ActorReferenceTarget is updated on index moved.
// ActorReference will transperently propigate updated indices.
struct ActorReferenceTarget final
{
	ActorIndex index;
};
class ActorReference final
{
	const ActorReferenceTarget* m_target;
public:
	ActorReference(const ActorReferenceTarget& target) : m_target(&target) { }
	ActorReference() : m_target(nullptr) { }
	ActorReference(Area& area, ActorIndex index);
	void setTarget(const ActorReferenceTarget& target) { m_target = &target; }
	void clear() { m_target = nullptr; }
	ActorIndex getIndex() const { assert(exists()); return m_target->index; }
	bool exists() const { return m_target != nullptr; }
	void load(const Json& data, Area& area);
	bool operator==(const ActorReference& other) const { return other.m_target == m_target; } 
	std::strong_ordering operator<=>(const ActorReference& other) const { return m_target <=> other.m_target; }
	size_t hash() const { return reinterpret_cast<size_t>(m_target); }
	struct Hash
	{
		size_t operator()(const ActorReference& reference) const { return reference.hash(); }
	};
};
inline void to_json(Json& data, const ActorReference& actor) { data = actor.getIndex(); }
class ActorReferences
{
protected:
	std::vector<ActorReference> data;
public:
	ActorReferences(const Json& data, Area& area) { for(const Json& irData : data) add(irData.get<ActorIndex>(), area); }
	ActorReferences() = default;
	[[nodiscard]] auto find(ActorReference ref) { return std::ranges::find(data, ref); }
	[[nodiscard]] auto find(ActorIndex index) { return std::ranges::find(data, index, [](const auto& ref){return ref.getIndex();}); }
	[[nodiscard]] auto find(ActorReference ref) const { return std::ranges::find(data, ref); }
	[[nodiscard]] auto find(ActorIndex index) const { return std::ranges::find(data, index, [](const auto& ref){return ref.getIndex();}); }
	[[nodiscard]] bool contains(ActorReference& ref) const { return find(ref) != data.end(); }
	[[nodiscard]] bool contains(ActorIndex index) const { return find(index) != data.end(); }
	template<typename Predicate>
	[[nodiscard]] std::vector<ActorReference>::iterator find(Predicate predicate) { return std::ranges::find_if(data, predicate); }
	template<typename Predicate>
	[[nodiscard]] std::vector<ActorReference>::const_iterator find(Predicate predicate) const { return std::ranges::find_if(data, predicate); }
	void add(ActorReference ref) { assert(!contains(ref)); data.push_back(ref); }
	void remove(ActorReference ref) { assert(contains(ref)); auto found = find(ref); std::swap(*found, data.back()); data.resize(data.size() - 1); }
	void add(ActorIndex index, Area& area) { data.emplace_back(area, index); }
	void remove(ActorIndex index) { auto found = find(index); std::swap(*found, data.back()); data.resize(data.size() - 1); }
	template<typename Predicate>
	void removeIf(Predicate predicate) { std::erase_if(data, predicate); }
	void clear() { data.clear(); }
	void maybeAdd(ActorIndex index, Area& area) { if(!contains(index)) data.emplace_back(area, index); }
	void maybeRemove(ActorIndex index) { auto found = find(index); if(found != data.end()) { std::swap(*found, data.back()); data.resize(data.size() - 1); } }
	void merge(ActorIndices indices, Area& area) { for(ActorIndex index : indices) maybeAdd(index, area); }
	[[nodiscard]] auto begin() { return data.begin(); }
	[[nodiscard]] auto end() { return data.end(); }
	[[nodiscard]] auto begin() const { return data.begin(); }
	[[nodiscard]] auto end() const { return data.end(); }
	[[nodiscard]] size_t size() { return data.size(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(ActorReferences, data);
};
// Each Item holds a reference target in a unique_ptr.
// Code outside the engine must use an itemReference rather then storing ItemIndex.
// ItemReferenceTarget is updated on index moved.
// ItemReference will transperently propigate updated indices.
struct ItemReferenceTarget final
{
	ItemIndex index;
};
class ItemReference final
{
	const ItemReferenceTarget* m_target;
public:
	ItemReference(const ItemReferenceTarget& target) : m_target(&target) { }
	ItemReference() : m_target(nullptr) { }
	ItemReference(Area& area, ItemIndex index);
	void setTarget(const ItemReferenceTarget& target) { m_target = &target; }
	void clear() { m_target = nullptr; }
	ItemIndex getIndex() const { assert(exists()); return m_target->index; }
	bool exists() const { return m_target != nullptr; }
	bool empty() const { return m_target == nullptr; }
	void load(const Json& data, Area& area);
	bool operator==(const ItemReference& other) const { return other.m_target == m_target; } 
	std::strong_ordering operator<=>(const ItemReference& other) const { return m_target <=> other.m_target; }
	size_t hash() const { return reinterpret_cast<size_t>(m_target); }
	struct Hash
	{
		size_t operator()(const ItemReference& reference) const { return reference.hash(); }
	};
};
inline void to_json(Json& data, const ItemReference& item) { data = item.getIndex(); }
class ItemReferences
{
protected:
	std::vector<ItemReference> data;
public:
	ItemReferences(const Json& data, Area& area) { for(const Json& irData : data) add(irData.get<ItemIndex>(), area); }
	ItemReferences() = default;
	[[nodiscard]] auto find(ItemReference ref) { return std::ranges::find(data, ref); }
	[[nodiscard]] auto find(ItemIndex index) { return std::ranges::find(data, index, [](const auto& ref){return ref.getIndex();}); }
	[[nodiscard]] auto find(ItemReference ref) const { return std::ranges::find(data, ref); }
	[[nodiscard]] auto find(ItemIndex index) const { return std::ranges::find(data, index, [](const auto& ref){return ref.getIndex();}); }
	[[nodiscard]] bool contains(ItemReference& ref) const { return find(ref) != data.end(); }
	[[nodiscard]] bool contains(ItemIndex index) const { return find(index) != data.end(); }
	template<typename Predicate>
	[[nodiscard]] std::vector<ItemReference>::iterator find(Predicate predicate) { return std::ranges::find_if(data, predicate); }
	template<typename Predicate>
	[[nodiscard]] std::vector<ItemReference>::const_iterator find(Predicate predicate) const { return std::ranges::find_if(data, predicate); }
	void add(ItemReference ref) { assert(!contains(ref)); data.push_back(ref); }
	void remove(ItemReference ref) { assert(contains(ref)); auto found = find(ref); std::swap(*found, data.back()); data.resize(data.size() - 1); }
	void add(ItemIndex index, Area& area) { data.emplace_back(area, index); }
	void remove(ItemIndex index) { auto found = find(index); std::swap(*found, data.back()); data.resize(data.size() - 1); }
	template<typename Predicate>
	void removeIf(Predicate predicate) { std::erase_if(data, predicate); }
	void clear() { data.clear(); }
	void addGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void removeGeneric(Area& area, ItemTypeId itemType, MaterialTypeId materialType, Quantity quantity);
	void maybeAdd(ItemIndex index, Area& area) { if(!contains(index)) data.emplace_back(area, index); }
	void maybeRemove(ItemReference ref) { auto found = find(ref); if(found != data.end()) { std::swap(*found, data.back()); data.resize(data.size() - 1); } }
	void maybeRemove(ItemIndex index) { auto found = find(index); if(found != data.end()) { std::swap(*found, data.back()); data.resize(data.size() - 1); } }
	void merge(ItemIndices indices, Area& area) { for(ItemIndex index : indices) maybeAdd(index, area); }
	template<typename Predicate>
	[[nodiscard]] bool contains(Predicate predicate) const { return find(predicate) != data.end(); }
	[[nodiscard]] auto begin() { return data.begin(); }
	[[nodiscard]] auto end() { return data.end(); }
	[[nodiscard]] auto begin() const { return data.begin(); }
	[[nodiscard]] auto end() const { return data.end(); }
	[[nodiscard]] size_t size() { return data.size(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(ItemReferences, data);
};
class ItemReferencesMaybeSorted : public ItemReferences
{
	bool sorted = false;
public:
	template<typename Sort>
		void sort(Sort sort) { assert(!sorted); std::ranges::sort(data, sort); sorted = true; }
	void add(ItemReference ref) { sorted = false; ItemReferences::add(ref); }
	void remove(ItemReference ref) { sorted = false; ItemReferences::remove(ref); }
	void add(ItemIndex index, Area& area) { sorted = false; ItemReferences::add(index, area); }
	void remove(ItemIndex index) { sorted = false; ItemReferences::remove(index); }
	[[nodiscard]] bool isSorted() const { return sorted; }
};
// Polymorphic reference wrapes either an actor or an item.
#include "actorOrItemIndex.h"
class ActorOrItemReference
{
	std::variant<std::monostate, ActorReference, ItemReference> m_reference;
public:
	void setActor(ActorReferenceTarget& target) { m_reference.emplace<1>(target); }
	void setItem(ItemReferenceTarget& target) { m_reference.emplace<2>(target); }
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
	[[nodiscard]] HasShapeIndex getIndex() const 
	{
		assert(exists());
		return isActor() ? HasShapeIndex::cast(std::get<1>(m_reference).getIndex()) : HasShapeIndex::cast(std::get<2>(m_reference).getIndex());
	}
	[[nodiscard]] ActorOrItemIndex getIndexPolymorphic() const
	{
		if(isActor())
			return ActorOrItemIndex::createForActor(ActorIndex::cast(getIndex()));
		else
			return ActorOrItemIndex::createForItem(ItemIndex::cast(getIndex()));
	}
	[[nodiscard]] static ActorOrItemReference createForActor(ActorReferenceTarget& target) {  ActorOrItemReference output; output.setActor(target); return output;}
	[[nodiscard]] static ActorOrItemReference createForItem(ItemReferenceTarget& target) {  ActorOrItemReference output; output.setItem(target); return output;}
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
inline void to_json(Json& data, const ActorOrItemReference& ref) { data = ref.getIndexPolymorphic(); }
