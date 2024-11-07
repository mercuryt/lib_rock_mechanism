#include "hasShapes.h"
#include "actors/actors.h"
#include "area.h"
#include "blocks/blocks.h"
#include "index.h"
#include "items/items.h"
#include "locationBuckets.h"
#include "plants.h"
#include "simulation.h"
#include "types.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <ranges>
HasShapes::HasShapes(Area& area) : m_area(area) { }
void HasShapes::create(const HasShapeIndex& index, const ShapeId& shape, const FactionId& faction, bool isStatic)
{
	assert(m_shape.size() > index);
	// m_location, m_facing, and m_underground are to be set by calling the derived class setLocation method.
	m_shape[index] = shape;
	m_faction[index] = faction;
	assert(m_blocks[index].empty());
	m_static.set(index, isStatic);
}
void HasShapes::resize(const HasShapeIndex& newSize)
{
	m_shape.resize(newSize);
	m_location.resize(newSize);
	m_facing.resize(newSize);
	m_faction.resize(newSize);
	m_blocks.resize(newSize);
	m_static.resize(newSize);
	m_underground.resize(newSize);
}
void HasShapes::moveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex)
{
	m_shape[newIndex] = m_shape[oldIndex];
	m_location[newIndex] = m_location[oldIndex];
	m_facing[newIndex] = m_facing[oldIndex];
	m_faction[newIndex] = m_faction[oldIndex];
	m_blocks[newIndex] = m_blocks[oldIndex];
	m_static.set(newIndex, m_static[oldIndex]);
	m_underground.set(newIndex, m_underground[oldIndex]);
}
void HasShapes::setStatic(const HasShapeIndex& index)
{
	assert(!m_static[index]);
	Blocks& blocks = m_area.getBlocks();
	for(auto& [x, y, z, v] : Shape::positionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex block = blocks.offset(m_location[index], x, y, z);
		CollisionVolume volume = CollisionVolume::create(v);
		blocks.shape_addStaticVolume(block, volume);
		blocks.shape_removeDynamicVolume(block, volume);
	}
	m_static.set(index, true);
}
void HasShapes::maybeSetStatic(const HasShapeIndex& index)
{
	if(!m_static[index])
		setStatic(index);
}
void HasShapes::unsetStatic(const HasShapeIndex& index)
{
	assert(m_static[index]);
	Blocks& blocks = m_area.getBlocks();
	for(auto& [x, y, z, v] : Shape::positionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex block = blocks.offset(m_location[index], x, y, z);
		CollisionVolume volume = CollisionVolume::create(v);
		blocks.shape_removeStaticVolume(block, volume);
		blocks.shape_addDynamicVolume(block, volume);
	}
	m_static.set(index, false);
}
void HasShapes::maybeUnsetStatic(const HasShapeIndex& index)
{
	if(m_static[index])
		unsetStatic(index);
}
void HasShapes::sortRange(const HasShapeIndex& begin, const HasShapeIndex& end)
{
	assert(false);
	assert(end > begin);
	//TODO: impliment sort with library rather then views::zip for clang / msvc support.
	/*
	auto zip = std::ranges::views::zip(m_shape, m_location, m_facing, m_faction, m_blocks, m_static, m_underground);
	std::ranges::sort(zip.begin() + begin, zip.end() + end, SpatialSort);
	*/
}
FactionId HasShapes::getFaction(const HasShapeIndex& index) const
{ 
	return m_faction[index];
}
bool HasShapes::isAdjacentToActor(const HasShapeIndex& index, const ActorIndex& actor) const
{
	return isAdjacentToActorAt(index, m_location[index], m_facing[index], actor);
}
bool HasShapes::isAdjacentToItem(const HasShapeIndex& index, const ItemIndex& item) const
{
	return isAdjacentToItemAt(index, m_location[index], m_facing[index], item);
}
bool HasShapes::isAdjacentToPlant(const HasShapeIndex& index, const PlantIndex& plant) const
{
	return isAdjacentToPlantAt(index, m_location[index], m_facing[index], plant);
}
bool HasShapes::isAdjacentToActorAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const ActorIndex& actor) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getActors().predicateForAnyAdjacentBlock(actor, predicate);
}
bool HasShapes::isAdjacentToItemAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const ItemIndex& item) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getItems().predicateForAnyAdjacentBlock(item, predicate);
}
bool HasShapes::isAdjacentToPlantAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const PlantIndex& plant) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getPlants().predicateForAnyAdjacentBlock(plant, predicate);
}
DistanceInBlocks HasShapes::distanceToActor(const HasShapeIndex& index, const ActorIndex& actor) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_area.getBlocks().distance(m_location[index], m_area.getActors().getLocation(actor));
}
DistanceInBlocks HasShapes::distanceToItem(const HasShapeIndex& index, const ItemIndex& item) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_area.getBlocks().distance(m_location[index], m_area.getItems().getLocation(item));
}
DistanceInBlocksFractional HasShapes::distanceToActorFractional(const HasShapeIndex& index, const ActorIndex& actor) const
{
	return m_area.getBlocks().distanceFractional(m_location[index], m_area.getActors().getLocation(actor));
}
DistanceInBlocksFractional HasShapes::distanceToItemFractional(const HasShapeIndex& index, const ItemIndex& item) const
{
	return m_area.getBlocks().distanceFractional(m_location[index], m_area.getItems().getLocation(item));
}
bool HasShapes::allOccupiedBlocksAreReservable(const HasShapeIndex& index, const FactionId& faction) const
{
	return allBlocksAtLocationAndFacingAreReservable(index, m_location[index], m_facing[index], faction);
}
bool HasShapes::isAdjacentToLocation(const HasShapeIndex& index, const BlockIndex& location) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return block == location; };
	return predicateForAnyAdjacentBlock(index, predicate);
}
bool HasShapes::isAdjacentToAny(const HasShapeIndex& index, const BlockIndices& locations) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return locations.contains(block); };
	return predicateForAnyAdjacentBlock(index, predicate);
}
bool HasShapes::isOnEdgeAt(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks](const BlockIndex block) { return blocks.isEdge(block); };
	return predicateForAnyOccupiedBlockAtLocationAndFacing(index, predicate, location, facing);
}
bool HasShapes::isOnEdge(const HasShapeIndex& index) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks](const BlockIndex block) { return blocks.isEdge(block); };
	return predicateForAnyOccupiedBlock(index, predicate);
}
bool HasShapes::predicateForAnyOccupiedBlock(const HasShapeIndex& index, std::function<bool(const BlockIndex&)> predicate) const
{
	assert(!m_blocks[index].empty());
	for(BlockIndex block : m_blocks[index])
		if(predicate(block))
			return true;
	return false;
}
bool HasShapes::predicateForAnyOccupiedBlockAtLocationAndFacing(const HasShapeIndex& index, std::function<bool(const BlockIndex&)> predicate, const BlockIndex& location, const Facing& facing) const
{
	for(BlockIndex occupied : const_cast<HasShapes&>(*this).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		if(predicate(occupied))
			return true;
	return false;
}
bool HasShapes::predicateForAnyAdjacentBlock(const HasShapeIndex& index, std::function<bool(const BlockIndex&)> predicate) const
{
	assert(m_location[index].exists());
	//TODO: cache this.
	for(auto [x, y, z] : Shape::adjacentPositionsWithFacing(m_shape[index], m_facing[index]))
	{
		BlockIndex block = m_area.getBlocks().offset(m_location[index], x, y, z);
		if(block.exists() && predicate(block))
			return true;
	}
	return false;
}
BlockIndices HasShapes::getAdjacentBlocks(const HasShapeIndex& index) const
{
	BlockIndices output;
	for(BlockIndex block : m_blocks[index])
		for(BlockIndex adjacent : m_area.getBlocks().getAdjacentWithEdgeAndCornerAdjacent(block))
			if(!m_blocks[index].contains(adjacent))
				output.addNonunique(adjacent);
	output.unique();
	return output;
}
ActorIndices HasShapes::getAdjacentActors(const HasShapeIndex& index) const
{
	ActorIndices output;
	for(BlockIndex block : getOccupiedAndAdjacentBlocks(index))
		for(const ActorIndex& actor : m_area.getBlocks().actor_getAll(block))
			output.maybeAdd(actor);
	output.remove(index.toActor());
	return output;
}
ItemIndices HasShapes::getAdjacentItems(const HasShapeIndex& index) const
{
	ItemIndices output;
	for(BlockIndex block : getOccupiedAndAdjacentBlocks(index))
		for(const ItemIndex& item : m_area.getBlocks().item_getAll(block))
			output.maybeAdd(item);
	output.remove(index.toItem());
	return output;
}
BlockIndices HasShapes::getOccupiedAndAdjacentBlocks(const HasShapeIndex& index) const
{
	return Shape::getBlocksOccupiedAndAdjacentAt(m_shape[index], m_area.getBlocks(), m_location[index], m_facing[index]);
}
BlockIndices HasShapes::getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing) const
{
	return Shape::getBlocksOccupiedAt(m_shape[index], m_area.getBlocks(), location, facing);
}
BlockIndices HasShapes::getAdjacentBlocksAtLocationWithFacing(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing) const
{
	return Shape::getBlocksWhichWouldBeAdjacentAt(m_shape[index], m_area.getBlocks(), location, facing);
}
BlockIndex HasShapes::getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)>& predicate) const
{
	return Shape::getBlockWhichWouldBeAdjacentAtWithPredicate(m_shape[index], m_area.getBlocks(), location, facing, predicate);
}
BlockIndex HasShapes::getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, std::function<bool(const BlockIndex&)>& predicate) const
{
	return Shape::getBlockWhichWouldBeOccupiedAtWithPredicate(m_shape[index], m_area.getBlocks(), location, facing, predicate);
}
BlockIndex HasShapes::getBlockWhichIsAdjacentWithPredicate(const HasShapeIndex& index, std::function<bool(const BlockIndex&)>& predicate) const
{
	return getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
BlockIndex HasShapes::getBlockWhichIsOccupiedWithPredicate(const HasShapeIndex& index, std::function<bool(const BlockIndex&)>& predicate) const
{
	return getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
ItemIndex HasShapes::getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, std::function<bool(const ItemIndex&)>& predicate) const
{
	for(auto [x, y, z] : Shape::adjacentPositionsWithFacing(m_shape[index], facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block.exists())
			for(ItemIndex item : m_area.getBlocks().item_getAll(block))
				if(predicate(item))
					return item;
	}
	return ItemIndex::null();
}
ItemIndex HasShapes::getItemWhichIsAdjacentWithPredicate(const HasShapeIndex& index, std::function<bool(const ItemIndex&)>& predicate) const
{
	return getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location[index], m_facing[index], predicate);
}
bool HasShapes::allBlocksAtLocationAndFacingAreReservable(const HasShapeIndex& index, const BlockIndex& location, const Facing& facing, const FactionId& faction) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex&)> predicate = [&blocks, faction](const BlockIndex& occupied) { return blocks.isReserved(occupied, faction); };
	return predicateForAnyOccupiedBlockAtLocationAndFacing(index, predicate, location, facing);
}
void HasShapes::log(const HasShapeIndex& index) const
{
	if(!m_location[index].exists())
	{
		std::cout << "*";
		return;
	}
	Point3D coordinates = m_area.getBlocks().getCoordinates(m_location[index]);
	std::cout << "[" << coordinates.x.get() << "," << coordinates.y.get() << "," << coordinates.z.get() << "]";
}
