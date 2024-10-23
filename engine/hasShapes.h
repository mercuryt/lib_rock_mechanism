/*
 * A non virtual base class for shareing code between Plant and CanMove.
 * CanMove is a nonvirtual base class for Actor and Item.
 */
#pragma once

#include "config.h"
#include "dataVector.h"
#include "shape.h"
#include "types.h"

struct Shape;
class Area;
struct DeserializationMemo;
class CanReserve;
struct Faction;

class HasShapes
{
protected:
	DataVector<ShapeId, HasShapeIndex> m_shape;
	DataVector<BlockIndex, HasShapeIndex> m_location;
	DataVector<Facing, HasShapeIndex> m_facing;
	DataVector<FactionId, HasShapeIndex> m_faction;
	DataVector<BlockIndices, HasShapeIndex> m_blocks;
	DataBitSet<HasShapeIndex> m_static;
	//TODO: Do we need m_underground?
	DataBitSet<HasShapeIndex> m_underground;
	Area& m_area;
	HasShapes(Area& area);
	HasShapes(const Json& data, DeserializationMemo& deserializationMemo);
	void create(const HasShapeIndex& index, const ShapeId& shape, const FactionId& faction, bool isStatic);
	void create(const HasShapeIndex& index, const Json& data, DeserializationMemo& deserializationMemo);
	void sortRange(const HasShapeIndex& begin, const HasShapeIndex& end);
	void resize(const HasShapeIndex& newSize);
	void moveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] HasShapeIndex getNextIndex();
public:
	void setFaction(const HasShapeIndex& index, FactionId faction);
	void setStatic(const HasShapeIndex& index, bool isStatic);
	void log(const HasShapeIndex& index) const;
	[[nodiscard]] size_t size() const { return m_shape.size(); }
	[[nodiscard]] ShapeId getShape(const HasShapeIndex& index) const { return m_shape[index]; }
	[[nodiscard]] BlockIndex getLocation(const HasShapeIndex& index) const { return m_location[index]; }
	[[nodiscard]] bool hasLocation(const HasShapeIndex& index) const { return getLocation(index).exists(); }
	[[nodiscard]] Facing getFacing(const HasShapeIndex& index) const { return m_facing[index]; }
	[[nodiscard]] const auto& getBlocks(const HasShapeIndex& index) const { return m_blocks[index]; }
	[[nodiscard]] FactionId getFactionId(const HasShapeIndex& index) const { return m_faction[index]; }
	[[nodiscard]] FactionId getFaction(const HasShapeIndex& index) const;
	[[nodiscard]] bool hasFaction(const HasShapeIndex& index) const { return m_faction[index].exists(); }
	[[nodiscard]] bool isStatic(const HasShapeIndex& index) const { return m_static[index]; }
	[[nodiscard]] bool isAdjacentToLocation(const HasShapeIndex& index, const BlockIndex& block) const;
	[[nodiscard]] bool isAdjacentToAny(const HasShapeIndex& index, const BlockIndices& block) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlock(const HasShapeIndex& index, std::function<bool(const BlockIndex&)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentBlock(const HasShapeIndex& index, std::function<bool(const BlockIndex&)> predicate) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlockAtLocationAndFacing(const HasShapeIndex& index, std::function<bool(const BlockIndex&)> predicate, const BlockIndex& location, const Facing& facing) const;
	[[nodiscard]] BlockIndices getAdjacentBlocks(const HasShapeIndex& index) const;
	[[nodiscard]] BlockIndices getOccupiedAndAdjacentBlocks(const HasShapeIndex& index) const;
	[[nodiscard]] ItemIndices getAdjacentItems(const HasShapeIndex& index) const;
	[[nodiscard]] ActorIndices getAdjacentActors(const HasShapeIndex& index) const;
	[[nodiscard]] BlockIndices getAdjacentBlocksAtLocationWithFacing(const HasShapeIndex& index, const BlockIndex& block, const Facing& facing) const;
	[[nodiscard]] BlockIndices getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing) const;
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const FactionId& faction) const;
	[[nodiscard]] bool allOccupiedBlocksAreReservable(const HasShapeIndex& index, const FactionId& faction) const;
	[[nodiscard]] bool isAdjacentToActor(const HasShapeIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItem(const HasShapeIndex& index, const ItemIndex& actor) const;
	[[nodiscard]] bool isAdjacentToPlant(const HasShapeIndex& index, const PlantIndex& plant) const;
	[[nodiscard]] bool isAdjacentToActorAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const ActorIndex& actor) const;
	[[nodiscard]] bool isAdjacentToItemAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const ItemIndex& item) const;
	[[nodiscard]] bool isAdjacentToPlantAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const PlantIndex& plant) const;
	[[nodiscard]] bool isOnEdgeAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing) const;
	[[nodiscard]] bool isOnEdge(const HasShapeIndex& index) const;
	[[nodiscard]] DistanceInBlocks distanceToActor(const HasShapeIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] DistanceInBlocks distanceToItem(const HasShapeIndex& index, const ItemIndex& item) const;
	[[nodiscard]] DistanceInBlocksFractional distanceToActorFractional(const HasShapeIndex& index, const ActorIndex& actor) const;
	[[nodiscard]] DistanceInBlocksFractional distanceToItemFractional(const HasShapeIndex& index, const ItemIndex& item) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentWithPredicate(const HasShapeIndex& index, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedWithPredicate(const HasShapeIndex& index, std::function<bool(const BlockIndex&)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, std::function<bool(const ItemIndex&)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentWithPredicate(const HasShapeIndex& index, std::function<bool(const ItemIndex&)>& predicate) const;
	[[nodiscard]] Area& getArea() { return m_area; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasShapes, m_shape, m_location, m_facing, m_faction, m_blocks, m_static, m_underground);
};
