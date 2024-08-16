#pragma once

#include "json.h"
#include "util.h"
#include "strongInteger.h"
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <vector>
#include <algorithm>
#include <unordered_set>
class PlantIndex;
class ItemIndex;
class ActorIndex;
class Simulation;
class ActorOrItemIndex;
class BlockIndex : public StrongInteger<BlockIndex, uint32_t>
{
	BlockIndex(uint32_t index) : StrongInteger<BlockIndex, uint32_t>(index) { }
public:
	static BlockIndex create(uint32_t index){ return BlockIndex(index); }
	[[nodiscard]] static BlockIndex null() { return BlockIndex(); }
	BlockIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const BlockIndex& index) const { return index.get(); } };
};
inline void to_json(Json& data, const BlockIndex& index) { data = index.get(); }
inline void from_json(const Json& data, BlockIndex& index) { index = BlockIndex::create(data.get<uint32_t>()); }
//TODO: this could be narrowed to uint16_t.
class VisionFacadeIndex : public StrongInteger<VisionFacadeIndex, uint32_t>
{
	VisionFacadeIndex(uint32_t index) : StrongInteger<VisionFacadeIndex, uint32_t>(index) { }
public:
	static VisionFacadeIndex create(uint32_t index){ return VisionFacadeIndex(index); }
	[[nodiscard]] static VisionFacadeIndex null() { return VisionFacadeIndex(); }
	VisionFacadeIndex() = default;
};
inline void to_json(Json& data, const VisionFacadeIndex& index) { data = index.get(); }
inline void from_json(const Json& data, VisionFacadeIndex& index) { index = VisionFacadeIndex::create(data.get<uint32_t>()); }
//TODO: this could be narrowed to uint16_t.
class PathRequestIndex : public StrongInteger<PathRequestIndex, uint32_t>
{
	PathRequestIndex(uint32_t index) : StrongInteger<PathRequestIndex, uint32_t>(index) { }
public:
	static PathRequestIndex create(uint32_t index){ return PathRequestIndex(index); }
	[[nodiscard]] static PathRequestIndex null() { return PathRequestIndex(); }
	PathRequestIndex() = default;
};
inline void to_json(Json& data, const PathRequestIndex& index) { data = index.get(); }
inline void from_json(const Json& data, PathRequestIndex& index) { index = PathRequestIndex::create(data.get<uint32_t>()); }
class HasShapeIndex : public StrongInteger<HasShapeIndex, uint32_t>
{
protected:
	HasShapeIndex(uint32_t index) : StrongInteger<HasShapeIndex, uint32_t>(index) { }
public:
	[[nodiscard]] static HasShapeIndex cast(const PlantIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ItemIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ActorIndex& o);
	[[nodiscard]] static HasShapeIndex null() { return StrongInteger<HasShapeIndex, uint32_t>::null(); }
	[[nodiscard]] static HasShapeIndex create(uint32_t index) { return index; }
	HasShapeIndex() = default;
	[[nodiscard]] PlantIndex toPlant() const;
	[[nodiscard]] ActorIndex toActor() const;
	[[nodiscard]] ItemIndex toItem() const;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
};
inline void to_json(Json& data, const HasShapeIndex& index) { data = index.get(); }
inline void from_json(const Json& data, HasShapeIndex& index) { index = HasShapeIndex::create(data.get<uint32_t>()); }
class PlantIndex final : public HasShapeIndex
{
protected:
	PlantIndex(uint32_t index) : HasShapeIndex(index) { }
public:
	[[nodiscard]] static PlantIndex cast(const HasShapeIndex& o) { return PlantIndex(o.get()); }
	[[nodiscard]] static PlantIndex null() { return PlantIndex(); }
	[[nodiscard]] static PlantIndex create(uint32_t index) { return index; }
	PlantIndex() = default;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
};
inline void to_json(Json& data, const PlantIndex& index) { data = index.get(); }
inline void from_json(const Json& data, PlantIndex& index) { index = PlantIndex::create(data.get<uint32_t>()); }

class ItemReference;
class Area;
class ItemIndex final : public HasShapeIndex
{
	ItemIndex(uint32_t index) : HasShapeIndex(index) { }
public:
	[[nodiscard]] static ItemIndex cast(const HasShapeIndex& o) { return ItemIndex(o.get()); }
	[[nodiscard]] static ItemIndex null() { return ItemIndex(); }
	[[nodiscard]] static ItemIndex create(uint32_t index) { return index; }
	[[nodiscard]] ActorOrItemIndex toActorOrItemIndex() const;
	[[nodiscard]] ItemReference toReference(Area& area) const;
	ItemIndex() = default;
};
inline void to_json(Json& data, const ItemIndex& index) { data = index.get(); }
inline void from_json(const Json& data, ItemIndex& index) { index = ItemIndex::create(data.get<uint32_t>()); }

class ActorReference;
class ActorIndex final : public HasShapeIndex
{
	ActorIndex(uint32_t index) : HasShapeIndex(index) { }
public:
	[[nodiscard]] static ActorIndex cast(const HasShapeIndex& o) { return ActorIndex(o.get()); }
	[[nodiscard]] static ActorIndex null() { return ActorIndex(); }
	[[nodiscard]] static ActorIndex create(uint32_t index) { return index; }
	[[nodiscard]] ActorOrItemIndex toActorOrItemIndex() const;
	[[nodiscard]] ActorReference toReference(Area& area) const;
	ActorIndex() = default;
};

// The offset required to lookup in one block's getAdjacentUnfiltered to find another block.
using AdjacentIndexWidth = int8_t;
class AdjacentIndex : public StrongInteger<AdjacentIndex, AdjacentIndexWidth>
{
public:
	AdjacentIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const AdjacentIndex& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AdjacentIndex& index) { data = index.get(); }
inline void from_json(const Json& data, AdjacentIndex& index) { index = AdjacentIndex::create(data.get<AdjacentIndexWidth>()); }
inline void to_json(Json& data, const ActorIndex& index) { data = index.get(); }
inline void from_json(const Json& data, ActorIndex& index) { index = ActorIndex::create(data.get<uint32_t>()); }
inline HasShapeIndex HasShapeIndex::cast(const PlantIndex& o) { return HasShapeIndex(o.get()); }
inline HasShapeIndex HasShapeIndex::cast(const ItemIndex& o) { return HasShapeIndex(o.get()); }
inline HasShapeIndex HasShapeIndex::cast(const ActorIndex& o) { return HasShapeIndex(o.get()); }
inline PlantIndex HasShapeIndex::toPlant() const { return PlantIndex::create(get()); }
inline ItemIndex HasShapeIndex::toItem() const { return ItemIndex::create(get()); }
inline ActorIndex HasShapeIndex::toActor() const { return ActorIndex::create(get()); }
using BlockIndices = StrongIntegerSet<BlockIndex>;
using ActorIndices = StrongIntegerSet<ActorIndex>;
using PlantIndices = StrongIntegerSet<PlantIndex>;
using ItemIndices = StrongIntegerSet<ItemIndex>;
using HasShapeIndices = StrongIntegerSet<HasShapeIndex>;
template <class StrongInteger>
class IndicesSetBase
{
	std::unordered_set<StrongInteger, typename StrongInteger::Hash> data;
public:
	void add(StrongInteger index) { data.insert(index); }
	void remove(StrongInteger index) { data.erase(index); }
	void clear() { data.clear(); }
	[[nodiscard]] bool empty() const { return data.empty(); }
	[[nodiscard]] bool contains(StrongInteger index) const { return data.contains(index); }
	[[nodiscard]] std::unordered_set<StrongInteger>::iterator begin() { return data.begin(); }
	[[nodiscard]] std::unordered_set<StrongInteger>::iterator end() { return data.end(); }
	[[nodiscard]] std::unordered_set<StrongInteger>::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] std::unordered_set<StrongInteger>::const_iterator end() const { return data.end(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(IndicesSetBase<StrongInteger>, data);
};
using BlockIndexSet = IndicesSetBase<BlockIndex>;
using ActorIndexSet = IndicesSetBase<ActorIndex>;
using ItemIndexSet = IndicesSetBase<ItemIndex>;
using PlantIndexSet = IndicesSetBase<PlantIndex>;
using HasShapeIndexSet = IndicesSetBase<HasShapeIndex>;
template <class StrongInteger, typename T, int count>
class IndicesArrayBase
{
	using Data = std::array<StrongInteger, count>;
	Data data;
public:
	void add(StrongInteger index) { assert(!contains(index)); assert(!full()); auto e = end(); (*e) = index; }
	void maybeAdd(StrongInteger index) { assert(!full()); if(!contains(index)){ auto e = end(); (*e) = index; } }
	void remove(StrongInteger index)
       	{
		auto found = find(index);
		assert(found != data.end());
		auto back = end() - 1;
		(*found) = *back;
		(*back) = StrongInteger::null(); 
	}
	void update(StrongInteger oldIndex, StrongInteger newIndex) { assert(!contains(newIndex)); auto found = find(oldIndex); assert(found != data.end()); (*found) = newIndex;}
	void clear() { assert(!empty()); data.fill(StrongInteger::null()); }
	void set(Data d) { data = d; }
	void pop_back() { auto e = end(); assert(e != data.begin()); (*e) = StrongInteger::null(); }
	[[nodiscard]] bool contains(StrongInteger index) const { return find(index) != data.end(); }
	[[nodiscard]] Data::iterator find(StrongInteger index) { return std::ranges::find(data, index); }
	[[nodiscard]] Data::const_iterator find(StrongInteger index) const { return std::ranges::find(data, index); }
	[[nodiscard]] Data::iterator begin() { return data.begin(); }
	[[nodiscard]] Data::iterator end() { return std::ranges::find(data, StrongInteger::null()); }
	[[nodiscard]] Data::const_iterator begin() const { return data.begin(); }
	[[nodiscard]] Data::const_iterator end() const { return std::ranges::find(data, StrongInteger::null()); }
	[[nodiscard]] StrongInteger front() const { return data.front(); }
	[[nodiscard]] StrongInteger back() const { auto e = end(); assert(e != data.begin()); return *(e - 1); }
	[[nodiscard]] bool full() const { return data.back() != StrongInteger::null(); }
	[[nodiscard]] bool empty() const { return data.front() == StrongInteger::null(); }
	[[nodiscard]] std::size_t size() const { return end() - begin(); }
	[[nodiscard]] Data& get() { return data; }
	[[nodiscard]] const Data& get() const { return data; }
};
template <class StrongInteger, typename T, int count>
inline void to_json(Json& data, const IndicesArrayBase<StrongInteger, T, count>& array) { data = array.get(); }
template <class StrongInteger, typename T, int count>
inline void from_json(const Json& data, IndicesArrayBase<StrongInteger, T, count>& array) { array.set(data.get<std::array<StrongInteger, count>>()); }
template<int count>
using ActorIndicesArray = IndicesArrayBase<ActorIndex, uint32_t, count>;
template<int count>
using ItemIndicesArray = IndicesArrayBase<ItemIndex, uint32_t, count>;

template<typename T>
using BlockIndexMap = std::unordered_map<BlockIndex, T, BlockIndex::Hash>;
