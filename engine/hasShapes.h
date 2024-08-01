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
	DataVector<const Shape*, HasShapeIndex> m_shape;
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
	void create(HasShapeIndex index, const Shape& shape, BlockIndex location, Facing facing, bool isStatic);
	void create(HasShapeIndex index, const Json& data, DeserializationMemo& deserializationMemo);
	void destroy(HasShapeIndex index);
	void sortRange(HasShapeIndex begin, HasShapeIndex end);
	void resize(HasShapeIndex newSize);
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] HasShapeIndex getNextIndex();
public:
	void setFaction(HasShapeIndex index, FactionId faction);
	void setStatic(HasShapeIndex index, bool isStatic);
	void log(HasShapeIndex index) const;
	[[nodiscard]] size_t size() const { return m_shape.size(); }
	[[nodiscard]] const Shape& getShape(HasShapeIndex index) const { return *m_shape.at(index); }
	[[nodiscard]] BlockIndex getLocation(HasShapeIndex index) const { return m_location.at(index); }
	[[nodiscard]] bool hasLocation(HasShapeIndex index) const { return getLocation(index).exists(); }
	[[nodiscard]] Facing getFacing(HasShapeIndex index) const { return m_facing.at(index); }
	[[nodiscard]] const auto& getBlocks(HasShapeIndex index) const { return m_blocks.at(index); }
	[[nodiscard]] FactionId getFactionId(HasShapeIndex index) { return m_faction.at(index); }
	[[nodiscard]] const Faction* getFaction(HasShapeIndex index) const;
	[[nodiscard]] bool hasFaction(HasShapeIndex index) const { return m_faction.at(index).exists(); }
	[[nodiscard]] bool isStatic(HasShapeIndex index) const { return m_static.at(index); }
	[[nodiscard]] bool isAdjacentToLocation(HasShapeIndex index, BlockIndex block) const;
	[[nodiscard]] bool isAdjacentToAny(HasShapeIndex index, BlockIndices block) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlock(HasShapeIndex index, std::function<bool(BlockIndex)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentBlock(HasShapeIndex index, std::function<bool(BlockIndex)> predicate) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlockAtLocationAndFacing(HasShapeIndex index, std::function<bool(const BlockIndex)> predicate, BlockIndex location, Facing facing) const;
	[[nodiscard]] BlockIndices getAdjacentBlocks(HasShapeIndex index) const;
	[[nodiscard]] BlockIndices getOccupiedAndAdjacentBlocks(HasShapeIndex index) const;
	[[nodiscard]] ItemIndices getAdjacentItems(HasShapeIndex index) const;
	[[nodiscard]] ActorIndices getAdjacentActors(HasShapeIndex index) const;
	[[nodiscard]] BlockIndices getAdjacentBlocksAtLocationWithFacing(HasShapeIndex index, BlockIndex block, Facing facing) const;
	[[nodiscard]] BlockIndices getBlocksWhichWouldBeOccupiedAtLocationAndFacing(HasShapeIndex index, BlockIndex location, Facing facing) const;
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(HasShapeIndex index, BlockIndex location, Facing facing, FactionId faction) const;
	[[nodiscard]] bool allOccupiedBlocksAreReservable(HasShapeIndex index, FactionId faction) const;
	[[nodiscard]] bool isAdjacentToActor(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItem(HasShapeIndex index, ItemIndex actor) const;
	[[nodiscard]] bool isAdjacentToPlant(HasShapeIndex index, PlantIndex plant) const;
	[[nodiscard]] bool isAdjacentToActorAt(HasShapeIndex index, BlockIndex location, Facing facing, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItemAt(HasShapeIndex index, BlockIndex location, Facing facing, ItemIndex item) const;
	[[nodiscard]] bool isAdjacentToPlantAt(HasShapeIndex index, BlockIndex location, Facing facing, PlantIndex plant) const;
	[[nodiscard]] bool isOnEdgeAt(HasShapeIndex index, BlockIndex location, Facing facing) const;
	[[nodiscard]] bool isOnEdge(HasShapeIndex index) const;
	[[nodiscard]] DistanceInBlocks distanceToActor(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] DistanceInBlocks distanceToItem(HasShapeIndex index, ItemIndex item) const;
	[[nodiscard]] DistanceInBlocksFractional distanceToActorFractional(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] DistanceInBlocksFractional distanceToItemFractional(HasShapeIndex index, ItemIndex item) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(HasShapeIndex index, BlockIndex location, Facing facing, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(HasShapeIndex index, BlockIndex location, Facing facing, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentWithPredicate(HasShapeIndex index, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedWithPredicate(HasShapeIndex index, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(HasShapeIndex index, BlockIndex location, Facing facing, std::function<bool(const ItemIndex)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentWithPredicate(HasShapeIndex index, std::function<bool(const ItemIndex)>& predicate) const;
	[[nodiscard]] Area& getArea() { return m_area; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(HasShapes, m_shape, m_location, m_facing, m_faction, m_blocks, m_static, m_underground);
};
