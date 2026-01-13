#pragma once

#include "../json.h"
#include "../util.h"
#include "../strongInteger.h"
#include "../dataStructures/strongVector.h"
#include "../dataStructures/smallSet.h"
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

using VisionFacadeIndexWidth = int;
class VisionFacadeIndex : public StrongInteger<VisionFacadeIndex, VisionFacadeIndexWidth, INT_MAX, 0> { };
void to_json(Json& data, const VisionFacadeIndex& index);
void from_json(const Json& data, VisionFacadeIndex& index);
using PathRequestIndexWidth = int;
class PathRequestIndex : public StrongInteger<PathRequestIndex, PathRequestIndexWidth, INT_MAX, 0> { };
void to_json(Json& data, const PathRequestIndex& index);
void from_json(const Json& data, PathRequestIndex& index);
// TODO: This type probably isn't needed. If it is it should have it's constrctors removed so it is trivially copyable.
class HasShapeIndex : public StrongInteger<HasShapeIndex, int, INT_MAX, 0>
{
public:
	HasShapeIndex() = default;
	HasShapeIndex(const PlantIndex& index);
	HasShapeIndex(const ItemIndex& index);
	HasShapeIndex(const ActorIndex& index);
	[[nodiscard]] static HasShapeIndex cast(const PlantIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ItemIndex& o);
	[[nodiscard]] static HasShapeIndex cast(const ActorIndex& o);
	[[nodiscard]] static HasShapeIndex null() { return StrongInteger<HasShapeIndex, int>::null(); }
	[[nodiscard]] PlantIndex toPlant() const;
	[[nodiscard]] ActorIndex toActor() const;
	[[nodiscard]] ItemIndex toItem() const;
};
void to_json(Json& data, const HasShapeIndex& index);
void from_json(const Json& data, HasShapeIndex& index);

class PlantIndex final : public StrongInteger<PlantIndex, int, INT_MAX, 0>
{
public:
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
	[[nodiscard]] static PlantIndex cast(const HasShapeIndex& index) { PlantIndex output; output.set(index.get()); return output; }
	struct Hash { [[nodiscard]] constexpr std::size_t operator()(const PlantIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const PlantIndex& index);
void from_json(const Json& data, PlantIndex& index);

class Area;
class ItemIndex final : public StrongInteger<ItemIndex, int, INT_MAX, 0>
{
public:
	[[nodiscard]] ActorOrItemIndex toActorOrItemIndex() const;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
	[[nodiscard]] static ItemIndex cast(const HasShapeIndex& index) { ItemIndex output; output.set(index.get()); return output; }
	struct Hash { [[nodiscard]] constexpr std::size_t operator()(const ItemIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const ItemIndex& index);
void from_json(const Json& data, ItemIndex& index);
// TODO: Check if 16 bit indices are faster then 32 bit.
using ActorIndexWidth = int16_t;
class ActorIndex final : public StrongInteger<ActorIndex, ActorIndexWidth, INT16_MAX, 0>
{
public:
	[[nodiscard]] ActorOrItemIndex toActorOrItemIndex() const;
	[[nodiscard]] HasShapeIndex toHasShape() const { return HasShapeIndex::create(get()); }
	[[nodiscard]] static ActorIndex cast(const HasShapeIndex& index) { ActorIndex output; output.set(index.get()); return output; }
	struct Hash { [[nodiscard]] constexpr std::size_t operator()(const ActorIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const ActorIndex& index);
void from_json(const Json& data, ActorIndex& index);

using AdjacentIndexWidth = int;
class AdjacentIndex : public StrongInteger<AdjacentIndex, AdjacentIndexWidth, 27, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AdjacentIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const AdjacentIndex& index);
void from_json(const Json& data, AdjacentIndex& index);

// TODO: Check if 16 bit indices are faster then 32 bit.
using ActorReferenceIndexWidth = int16_t;
class ActorReferenceIndex : public StrongInteger<ActorReferenceIndex, ActorReferenceIndexWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ActorReferenceIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const ActorReferenceIndex& index);
void from_json(const Json& data, ActorReferenceIndex& index);

using ItemReferenceIndexWidth = int;
class ItemReferenceIndex : public StrongInteger<ItemReferenceIndex, ItemReferenceIndexWidth, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ItemReferenceIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const ItemReferenceIndex& index);
void from_json(const Json& data, ItemReferenceIndex& index);

using RTreeNodeIndexWidth = int;
class RTreeNodeIndex final : public StrongInteger<RTreeNodeIndex, RTreeNodeIndexWidth, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const RTreeNodeIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const RTreeNodeIndex& index);
void from_json(const Json& data, RTreeNodeIndex& index);

// uint64 is used because that is what is returned from BitSet64::getNext().
using RTreeArrayIndexWidth = uint64_t;
// TODO: 65 is used rathern then Config::rtreeNodeSize to prevent include cycle, break by moving RTReeArrayIndex into a different file?
// 66 is used as null rather then 65 as 65 is neccessary for defining a range to the end.
// 65 is needed rather then 64 because the cuboid candidates used in insertion have a size of nodeSize + 1.
class RTreeArrayIndex final : public StrongInteger<RTreeArrayIndex, RTreeArrayIndexWidth, 70, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const RTreeArrayIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const RTreeArrayIndex& index);
void from_json(const Json& data, RTreeArrayIndex& index);

class SquadIndex final : public StrongInteger<SquadIndex, int, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SquadIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const SquadIndex& index);
void from_json(const Json& data, SquadIndex& index);

class SquadFormationIndex final : public StrongInteger<SquadFormationIndex, int, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SquadFormationIndex& index) const { return index.get(); } };
};
void to_json(Json& data, const SquadFormationIndex& index);
void from_json(const Json& data, SquadFormationIndex& index);

inline HasShapeIndex HasShapeIndex::cast(const PlantIndex& o) { return HasShapeIndex::create(o.get()); }
inline HasShapeIndex HasShapeIndex::cast(const ItemIndex& o) { return HasShapeIndex::create(o.get()); }
inline HasShapeIndex HasShapeIndex::cast(const ActorIndex& o) { return HasShapeIndex::create(o.get()); }
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