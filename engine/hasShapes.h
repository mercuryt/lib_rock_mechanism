/*
 * A non virtual base class for shareing code between Plant and CanMove.
 * CanMove is a nonvirtual base class for Actor and Item.
 */
#pragma once

#include "config.h"
#include "types.h"
#include "util.h"
#include "../lib/dynamic_bitset.hpp"

#include <ranges>

struct Shape;
struct Faction;
class Area;
class DeserializationMemo;
class CanReserve;

class HasShapes
{
	std::vector<HasShapeIndex> m_freeSlots;
protected:
	std::unordered_set<HasShapeIndex> m_onSurface;
	std::vector<const Shape*> m_shape;
	std::vector<BlockIndex> m_location;
	std::vector<Facing> m_facing;
	std::vector<Faction*> m_faction;
	std::vector<std::unordered_set<BlockIndex>> m_blocks;
	sul::dynamic_bitset<> m_static;
	//TODO: Do we need m_underground?
	sul::dynamic_bitset<> m_underground;
	Area& m_area;
	HasShapes(Area& area);
	HasShapes(const Json& data, DeserializationMemo& deserializationMemo);
	void create(HasShapeIndex index, const Shape& shape, BlockIndex location, Facing facing, bool isStatic);
	void create(HasShapeIndex index, const Json& data, DeserializationMemo& deserializationMemo);
	void destroy(HasShapeIndex index);
	void setLocation(HasShapeIndex index, BlockIndex location);
	void sortRange(HasShapeIndex begin, HasShapeIndex end);
	virtual void resize(HasShapeIndex newSize);
	virtual void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	[[nodiscard]] virtual bool indexCanBeMoved(HasShapeIndex index) const;
	[[nodiscard]] virtual Json toJson() const;
	[[nodiscard]] HasShapeIndex getNextIndex();
public:
	void setFacing(Facing facing);
	void setFaction(HasShapeIndex index, Faction& faction);
	void setStatic(HasShapeIndex index, bool isStatic);
	void updateIsOnSurface(HasShapeIndex index, BlockIndex block);
	void log(HasShapeIndex index) const;
	void setShape(HasShapeIndex index, const Shape& shape);
	[[nodiscard]] const Shape& getShape(HasShapeIndex index) const;
	[[nodiscard]] BlockIndex getLocation(HasShapeIndex index) const;
	[[nodiscard]] Facing getFacing(HasShapeIndex index) const;
	[[nodiscard]] std::unordered_set<BlockIndex>& getBlocks(HasShapeIndex index) const;
	[[nodiscard]] Faction* getFaction(HasShapeIndex index);
	[[nodiscard]] bool isStatic(HasShapeIndex index) const { return m_static[index]; }
	[[nodiscard]] bool isAdjacentToLocation(HasShapeIndex index, BlockIndex block) const;
	[[nodiscard]] bool isAdjacentToAny(HasShapeIndex index, std::unordered_set<BlockIndex> block) const;
	[[nodiscard]] bool predicateForAnyOccupiedBlock(HasShapeIndex index, std::function<bool(BlockIndex)> predicate) const;
	[[nodiscard]] bool predicateForAnyAdjacentBlock(HasShapeIndex index, std::function<bool(BlockIndex)> predicate) const;
	[[nodiscard]] std::unordered_set<BlockIndex> getAdjacentBlocks(HasShapeIndex index) const;
	[[nodiscard]] std::unordered_set<BlockIndex> getOccupiedAndAdjacentBlocks(HasShapeIndex index) const;
	[[nodiscard]] std::unordered_set<ItemIndex> getAdjacentItems(HasShapeIndex index) const;
	[[nodiscard]] std::unordered_set<ActorIndex> getAdjacentActors(HasShapeIndex index) const;
	[[nodiscard]] std::vector<BlockIndex> getAdjacentBlocksAtLocationWithFacing(HasShapeIndex index, BlockIndex block, Facing facing) const;
	[[nodiscard]] std::vector<BlockIndex> getBlocksWhichWouldBeOccupiedAtLocationAndFacing(HasShapeIndex index, BlockIndex location, Facing facing) const;
	[[nodiscard]] bool allBlocksAtLocationAndFacingAreReservable(HasShapeIndex index, BlockIndex location, Facing facing, Faction& faction) const;
	[[nodiscard]] bool allOccupiedBlocksAreReservable(HasShapeIndex index, Faction& faction) const;
	[[nodiscard]] bool isAdjacentToActor(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItem(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToPlant(HasShapeIndex index, PlantIndex plant) const;
	[[nodiscard]] bool isAdjacentToActorAt(HasShapeIndex index, BlockIndex location, Facing facing, ActorIndex actor) const;
	[[nodiscard]] bool isAdjacentToItemAt(HasShapeIndex index, BlockIndex location, Facing facing, ItemIndex item) const;
	[[nodiscard]] bool isAdjacentToPlantAt(HasShapeIndex index, BlockIndex location, Facing facing, PlantIndex plant) const;
	[[nodiscard]] DistanceInBlocks distanceToActor(HasShapeIndex index, ActorIndex actor) const;
	[[nodiscard]] DistanceInBlocks distanceToItem(HasShapeIndex index, ItemIndex item) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(HasShapeIndex index, BlockIndex location, Facing facing, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(HasShapeIndex index, BlockIndex location, Facing facing, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsAdjacentWithPredicate(HasShapeIndex index, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] BlockIndex getBlockWhichIsOccupiedWithPredicate(HasShapeIndex index, std::function<bool(BlockIndex)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(HasShapeIndex index, BlockIndex location, Facing facing, std::function<bool(const ItemIndex)>& predicate) const;
	[[nodiscard]] ItemIndex getItemWhichIsAdjacentWithPredicate(HasShapeIndex index, std::function<bool(const ItemIndex)>& predicate) const;
	[[nodiscard]] std::vector<HasShapeIndex> getAll() const;
};
