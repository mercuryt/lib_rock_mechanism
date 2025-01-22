/*
 * A non virtual base class for shareing code between Plant and CanMove.
 * CanMove is a nonvirtual base class for Actor and Item.
 */
#pragma once

#include "config.h"
#include "dataVector.h"
#include "shape.h"
#include "types.h"
#include "hasShapeTypes.h"

struct Shape;
class Area;
struct DeserializationMemo;
class CanReserve;
struct Faction;

template<class Derived, class Index>
class HasShapes
{
protected:
	DataVector<ShapeId, Index> m_shape;
	DataVector<BlockIndex, Index> m_location;
	DataVector<Facing4, Index> m_facing;
	DataVector<FactionId, Index> m_faction;
	DataVector<OccupiedBlocksForHasShape, Index> m_blocks;
	DataBitSet<Index> m_static;
	//TODO: Do we need m_underground?
	DataBitSet<Index> m_underground;
	Area& m_area;
	HasShapes(Area& area);
	void create(const Index& index, const ShapeId& shape, const FactionId& faction, bool isStatic);
	std::vector<std::pair<uint32_t, Index>> getSortOrder(const Index& begin, const Index& end);
	void resize(const Index& newSize);
public:
	template<typename Action>
	void forEachDataHasShapes(Action&& action);
	void setStatic(const Index& index);
	void maybeSetStatic(const Index& index);
	void unsetStatic(const Index& index);
	void maybeUnsetStatic(const Index& index);
	void log(const Index& index) const;
	[[nodiscard]] size_t size() const { return m_shape.size(); }
	[[nodiscard]] ShapeId getShape(const Index& index) const { return m_shape[index]; }
	[[nodiscard]] BlockIndex getLocation(const Index& index) const { return m_location[index]; }
	[[nodiscard]] bool hasLocation(const Index& index) const { return getLocation(index).exists(); }
	[[nodiscard]] Facing4 getFacing(const Index& index) const { return m_facing[index]; }
	[[nodiscard]] const auto& getBlocks(const Index& index) const { return m_blocks[index]; }
	[[nodiscard]] FactionId getFactionId(const Index& index) const { return m_faction[index]; }
	[[nodiscard]] FactionId getFaction(const Index& index) const;
	[[nodiscard]] bool hasFaction(const Index& index) const { return m_faction[index].exists(); }
	[[nodiscard]] bool isStatic(const Index& index) const { return m_static[index]; }
	[[nodiscard]] bool isAdjacentToLocation(const Index& index, const BlockIndex& block) const;
	[[nodiscard]] bool isAdjacentToAny(const Index& index, const BlockIndices& block) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlock(const Index& index, std::function<bool(const BlockIndex&)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentBlock(const Index& index, std::function<bool(const BlockIndex&)> predicate) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlockAtLocationAndFacing(const Index& index, std::function<bool(const BlockIndex&)> predicate, const BlockIndex& location, const Facing4& facing) const;
	[[nodiscard]] BlockIndices getAdjacentBlocks(const Index& index) const;
	[[nodiscard]] BlockIndices getOccupiedAndAdjacentBlocks(const Index& index) const;
	[[nodiscard]] ItemIndices getAdjacentItems(const Index& index) const;
	[[nodiscard]] ActorIndices getAdjacentActors(const Index& index) const;
	[[nodiscard]] BlockIndices getAdjacentBlocksAtLocationWithFacing(const Index& index, const BlockIndex& block, const Facing4& facing) const;
	// TODO: return OccuipedBlocksForHasShape
	[[nodiscard]] BlockIndices getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const Index& index, const BlockIndex& location, const Facing4& facing) const;
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(const Index& index, const BlockIndex& location, const Facing4& facing, const FactionId& faction) const;
	[[nodiscard]] bool allOccupiedBlocksAreReservable(const Index& index, const FactionId& faction) const;
	[[nodiscard]] bool isAdjacentToActor(const Index& index, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItem(const Index& index, const ItemIndex& actor) const;
	[[nodiscard]] bool isAdjacentToPlant(const Index& index, const PlantIndex& plant) const;
	[[nodiscard]] bool isAdjacentToActorAt(const Index& index, const BlockIndex& location, const Facing4& facing, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItemAt(const Index& index, const BlockIndex& location, const Facing4& facing, const ItemIndex& item) const;
	[[nodiscard]] bool isAdjacentToPlantAt(const Index& index, const BlockIndex& location, const Facing4& facing, const PlantIndex& plant) const;
	[[nodiscard]] bool isOnEdgeAt(const Index& index, const BlockIndex& location, const Facing4& facing) const;
	[[nodiscard]] bool isOnEdge(const Index& index) const;
	[[nodiscard]] DistanceInBlocks distanceToActor(const Index& index, const ActorIndex& actor) const;
	[[nodiscard]] DistanceInBlocks distanceToItem(const Index& index, const ItemIndex& item) const;
	[[nodiscard]] DistanceInBlocksFractional distanceToActorFractional(const Index& index, const ActorIndex& actor) const;
	[[nodiscard]] DistanceInBlocksFractional distanceToItemFractional(const Index& index, const ItemIndex& item) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const Index& index, const BlockIndex& location, const Facing4& facing, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentWithPredicate(const Index& index, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedWithPredicate(const Index& index, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const Index& index, const BlockIndex& location, const Facing4& facing, std::function<bool(const ItemIndex&)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentWithPredicate(const Index& index, std::function<bool(const ItemIndex&)>& predicate) const;
	[[nodiscard]] Area& getArea() { return m_area; }
	[[nodiscard]] const Area& getArea() const { return m_area; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasShapes, m_shape, m_location, m_facing, m_faction, m_blocks, m_static, m_underground);
};
