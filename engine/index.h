#pragma once

#include "json.h"
#include "util.h"
#include "strongInteger.h"
#include "dataStructures/strongVector.h"
#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <vector>
#include <algorithm>
class PlantIndex;
class ItemIndex;
class ActorIndex;
class Simulation;
class ActorOrItemIndex;
using BlockIndexWidth = uint32_t;
class BlockIndex : public StrongInteger<BlockIndex, BlockIndexWidth>
{
	BlockIndex(BlockIndexWidth index) : StrongInteger<BlockIndex, BlockIndexWidth>(index) { }
public:
	static BlockIndex create(BlockIndexWidth index){ return BlockIndex(index); }
	[[nodiscard]] static BlockIndex null() { return BlockIndex(); }
	BlockIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const BlockIndex& index) const { return index.get(); } };
	// A no inline version of create for use in debug console.
	[[nodiscard]] static __attribute__((noinline)) BlockIndex dbg(const BlockIndexWidth& value);
};
inline void to_json(Json& data, const BlockIndex& index) { data = index.get(); }
inline void from_json(const Json& data, BlockIndex& index) { index = BlockIndex::create(data.get<BlockIndexWidth>()); }

using BlockIndexChunkedWidth = uint32_t;
class BlockIndexChunked : public StrongInteger<BlockIndexChunked, BlockIndexChunkedWidth>
{
	BlockIndexChunked(BlockIndexChunkedWidth index) : StrongInteger<BlockIndexChunked, BlockIndexChunkedWidth>(index) { }
public:
	static BlockIndexChunked create(BlockIndexChunkedWidth index){ return BlockIndexChunked(index); }
	[[nodiscard]] static BlockIndexChunked null() { return BlockIndexChunked(); }
	BlockIndexChunked() = default;
	struct Hash { [[nodiscard]] size_t operator()(const BlockIndexChunked& index) const { return index.get(); } };
};
inline void to_json(Json& data, const BlockIndexChunked& index) { data = index.get(); }
inline void from_json(const Json& data, BlockIndexChunked& index) { index = BlockIndexChunked::create(data.get<BlockIndexChunkedWidth>()); }
//TODO: this could be narrowed to uint16_t.

using VisionFacadeIndexWidth = uint32_t;
class VisionFacadeIndex : public StrongInteger<VisionFacadeIndex, VisionFacadeIndexWidth>
{
	VisionFacadeIndex(VisionFacadeIndexWidth index) : StrongInteger<VisionFacadeIndex, VisionFacadeIndexWidth>(index) { }
public:
	static VisionFacadeIndex create(VisionFacadeIndexWidth index){ return VisionFacadeIndex(index); }
	[[nodiscard]] static VisionFacadeIndex null() { return VisionFacadeIndex(); }
	VisionFacadeIndex() = default;
};
inline void to_json(Json& data, const VisionFacadeIndex& index) { data = index.get(); }
inline void from_json(const Json& data, VisionFacadeIndex& index) { index = VisionFacadeIndex::create(data.get<VisionFacadeIndexWidth>()); }
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
	HasShapeIndex(const PlantIndex& index);
	HasShapeIndex(const ItemIndex& index);
	HasShapeIndex(const ActorIndex& index);
	[[nodiscard]] static HasShapeIndex cast(const PlantIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ItemIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ActorIndex& o);
	[[nodiscard]] static HasShapeIndex null() { return StrongInteger<HasShapeIndex, uint32_t>::null(); }
	[[nodiscard]] static HasShapeIndex create(uint32_t index) { return index; }
	HasShapeIndex() = default;
	[[nodiscard]] PlantIndex toPlant() const;
	[[nodiscard]] ActorIndex toActor() const;
	[[nodiscard]] ItemIndex toItem() const;
};
inline void to_json(Json& data, const HasShapeIndex& index) { data = index.get(); }
inline void from_json(const Json& data, HasShapeIndex& index) { index = HasShapeIndex::create(data.get<uint32_t>()); }
class PlantIndex final : public StrongInteger<PlantIndex, uint32_t>
{
protected:
	PlantIndex(uint32_t index) : StrongInteger<PlantIndex, uint32_t>(index) { }
public:
	[[nodiscard]] static PlantIndex cast(const HasShapeIndex& o) { return PlantIndex(o.get()); }
	[[nodiscard]] static PlantIndex null() { return PlantIndex(); }
	[[nodiscard]] static PlantIndex create(uint32_t index) { return index; }
	PlantIndex() = default;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
};
inline void to_json(Json& data, const PlantIndex& index) { data = index.get(); }
inline void from_json(const Json& data, PlantIndex& index) { index = PlantIndex::create(data.get<uint32_t>()); }

class Area;
class ItemIndex final : public StrongInteger<ItemIndex, uint32_t>
{
	ItemIndex(uint32_t index) : StrongInteger<ItemIndex, uint32_t>(index) { }
public:
	[[nodiscard]] static ItemIndex cast(const HasShapeIndex& o) { return ItemIndex(o.get()); }
	[[nodiscard]] static ItemIndex null() { return ItemIndex(); }
	[[nodiscard]] static ItemIndex create(uint32_t index) { return index; }
	[[nodiscard]] ActorOrItemIndex toActorOrItemIndex() const;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
	ItemIndex() = default;
};
inline void to_json(Json& data, const ItemIndex& index) { data = index.get(); }
inline void from_json(const Json& data, ItemIndex& index) { index = ItemIndex::create(data.get<uint32_t>()); }

class ActorIndex final : public StrongInteger<ActorIndex, uint32_t>
{
	ActorIndex(uint32_t index) : StrongInteger<ActorIndex, uint32_t>(index) { }
public:
	[[nodiscard]] static ActorIndex cast(const HasShapeIndex& o) { return ActorIndex(o.get()); }
	[[nodiscard]] static ActorIndex null() { return ActorIndex(); }
	[[nodiscard]] static ActorIndex create(uint32_t index) { return index; }
	[[nodiscard]] ActorOrItemIndex toActorOrItemIndex() const;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
	ActorIndex() = default;
};
inline void to_json(Json& data, const ActorIndex& index) { data = index.get(); }
inline void from_json(const Json& data, ActorIndex& index) { index = ActorIndex::create(data.get<uint32_t>()); }

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

using ActorReferenceIndexWidth = uint32_t;
class ActorReferenceIndex : public StrongInteger<ActorReferenceIndex, ActorReferenceIndexWidth>
{
public:
	ActorReferenceIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const ActorReferenceIndex& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ActorReferenceIndex& index) { data = index.get(); }
inline void from_json(const Json& data, ActorReferenceIndex& index) { index = ActorReferenceIndex::create(data.get<ActorReferenceIndexWidth>()); }

using ItemReferenceIndexWidth = uint32_t;
class ItemReferenceIndex : public StrongInteger<ItemReferenceIndex, ItemReferenceIndexWidth>
{
public:
	ItemReferenceIndex() = default;
	struct Hash { [[nodiscard]] size_t operator()(const ItemReferenceIndex& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ItemReferenceIndex& index) { data = index.get(); }
inline void from_json(const Json& data, ItemReferenceIndex& index) { index = ItemReferenceIndex::create(data.get<ItemReferenceIndexWidth>()); }

inline HasShapeIndex HasShapeIndex::cast(const PlantIndex& o) { return HasShapeIndex(o.get()); }
inline HasShapeIndex HasShapeIndex::cast(const ItemIndex& o) { return HasShapeIndex(o.get()); }
inline HasShapeIndex HasShapeIndex::cast(const ActorIndex& o) { return HasShapeIndex(o.get()); }
inline PlantIndex HasShapeIndex::toPlant() const { return PlantIndex::create(get()); }
inline ItemIndex HasShapeIndex::toItem() const { return ItemIndex::create(get()); }
inline ActorIndex HasShapeIndex::toActor() const { return ActorIndex::create(get()); }
template<typename Index>
class TrackedIndexData;
template<typename Index>
class TrackedIndex
{
	using This = TrackedIndex<Index>;
	Index m_index;
	TrackedIndexData<Index>* m_data;
public:
	TrackedIndex() = default;
	TrackedIndex(Index index, TrackedIndexData<Index>& data) :
		m_index(index),
		m_data(&data)
	{
		m_data.record(index, *this);
	}
	TrackedIndex(const This& other) :
		m_index(other.m_index),
		m_data(other.m_data)
	{
		m_data.record(index, *this);
	}
	TrackedIndex(This&&  other) noexcept :
		m_index(other.m_index),
		m_data(other.m_data)
	{
		m_data.record(index, *this);
	}
	TrackedIndex operator=(const This& other)
	{
		if(m_data != nullptr)
		{
			assert(m_data == other.m_data);
			m_data.erase(*this);
		}
		else
			m_data = &other.m_data;
		m_index = other.m_index;
		m_data.record(m_index, *this);
	}
	TrackedIndex operator=(This&& other)
	{
		if(m_data != nullptr)
		{
			assert(m_data == other.m_data);
			m_data.erase(*this);
		}
		else
			m_data = &other.m_data;
		m_index = other.m_index;
		m_data.record(m_index, *this);
	}
	~TrackedIndex()
	{
		m_data.erase(m_index, *this);
	}
};
template<typename Index>
class TrackedIndexData
{
	StrongVector<SmallSet<TrackedIndex<Index>*>, Index> m_data;
public:
	void record(TrackedIndex<Index>& trackedIndex)
	{
		m_data[trackedIndex.m_index].insert(&trackedIndex);
	}
	void erase(TrackedIndex<Index>& trackedIndex)
	{
		m_data[trackedIndex.m_index].erase(&trackedIndex);
	}
	[[nodiscard]] bool empty(const Index& index){ return m_data[index].empty(); }
	[[nodiscard]] auto get(const Index& index) -> TrackedIndex<Index> { return {index, *this}; }
};