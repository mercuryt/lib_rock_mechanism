#include "hasShapes.h"
#include "actors/actors.h"
#include "area.h"
#include "blocks/blocks.h"
#include "index.h"
#include "items/items.h"
#include "json.h"
#include "config.h"
#include "locationBuckets.h"
#include "plants.h"
#include "simulation.h"
#include "types.h"
#include "util.h"
#include <cassert>
#include <cstdint>
#include <iostream>
#include <ranges>
HasShapes::HasShapes(Area& area) : m_area(area) { }
HasShapeIndex HasShapes::getNextIndex()
{
	HasShapeIndex index = HasShapeIndex::create(m_shape.size());
	// Virtual metod call.
	resize(index);
	return index;
}
void HasShapes::create(HasShapeIndex index, const Shape& shape, BlockIndex location, Facing facing, bool isStatic)
{
	assert(m_shape.size() > index);
	m_shape.at(index) = &shape;
	m_location.at(index) = location;
	m_facing.at(index) = facing;
	m_static.set(index, isStatic);
}
void HasShapes::resize(HasShapeIndex newSize)
{
	m_shape.resize(newSize());
	m_location.resize(newSize());
	m_facing.resize(newSize());
	m_blocks.resize(newSize());
	m_static.resize(newSize());
	m_underground.resize(newSize());
}
void HasShapes::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	m_shape.at(newIndex) = m_shape.at(oldIndex);
	m_location.at(newIndex) = m_location.at(oldIndex);
	m_facing.at(newIndex) = m_facing.at(oldIndex);
	m_blocks.at(newIndex) = m_blocks.at(oldIndex);
	m_static.set(newIndex, m_static.at(oldIndex));
	m_underground.set(newIndex, m_underground.at(oldIndex));
}
void HasShapes::destroy(HasShapeIndex index)
{
	// Copy last plant over this slot.
	auto oldIndex = HasShapeIndex::create(size() - 1);
	if(size() != 1)
		// Virtual call.
		moveIndex(index, oldIndex);
	// Truncate empty slot.
	// Virtual call.
	resize(oldIndex);
}
//TODO: Move this to derived.
void HasShapes::setShape(HasShapeIndex index, const Shape& shape)
{
	BlockIndex location = m_location.at(index);
	if(location.exists())
	{
		exit(index);
		//TODO: virtual function?
		//if(isActor())
			//m_area->getBlocks().getLocationBucket(location).erase(static_cast<Actor&>(*this));
	}
	m_shape.at(index) = &shape;
	if(location.exists())
		setLocation(index, location);
}
void HasShapes::setStatic(HasShapeIndex index, bool isTrue)
{
	assert(!m_static.at(index));
	if(isTrue)
		for(auto& [x, y, z, v] : m_shape.at(index)->positionsWithFacing(m_facing.at(index)))
		{
			BlockIndex block = m_area.getBlocks().offset(m_location.at(index), x, y, z);
			m_area.getBlocks().shape_addStaticVolume(block, v);
		}
	else
		for(auto& [x, y, z, v] : m_shape.at(index)->positionsWithFacing(m_facing.at(index)))
		{
			BlockIndex block = m_area.getBlocks().offset(m_location.at(index), x, y, z);
			m_area.getBlocks().shape_removeStaticVolume(block, v);
		}
	m_static.set(index);
}
void HasShapes::updateIsOnSurface(HasShapeIndex index, BlockIndex block)
{
	assert(block == getLocation(index));

	if(m_area.getBlocks().isOnSurface(block))
		m_onSurface.add(index);
	else
		m_onSurface.remove(index);
}
void HasShapes::sortRange(HasShapeIndex begin, HasShapeIndex end)
{
	assert(false);
	assert(end > begin);
	//TODO: impliment sort with library rather then views::zip for clang / msvc support.
	/*
	auto zip = std::ranges::views::zip(m_shape, m_location, m_facing, m_faction, m_blocks, m_static, m_underground);
	std::ranges::sort(zip.begin() + begin, zip.end() + end, SpatialSort);
	*/
}
const Faction* HasShapes::getFaction(HasShapeIndex index) const
{ 
	if(m_faction.at(index) == FACTION_ID_MAX)
		return nullptr;
	return &m_area.m_simulation.m_hasFactions.getById(m_faction.at(index));
}
bool HasShapes::isAdjacentToActor(HasShapeIndex index, ActorIndex actor) const
{
	return isAdjacentToActorAt(index, m_location.at(index), m_facing.at(index), actor);
}
bool HasShapes::isAdjacentToItem(HasShapeIndex index, ItemIndex item) const
{
	return isAdjacentToItemAt(index, m_location.at(index), m_facing.at(index), item);
}
bool HasShapes::isAdjacentToPlant(HasShapeIndex index, PlantIndex plant) const
{
	return isAdjacentToPlantAt(index, m_location.at(index), m_facing.at(index), plant);
}
bool HasShapes::isAdjacentToActorAt(HasShapeIndex index, BlockIndex location, Facing facing, ActorIndex actor) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getActors().predicateForAnyAdjacentBlock(actor, predicate);
}
bool HasShapes::isAdjacentToItemAt(HasShapeIndex index, BlockIndex location, Facing facing, ItemIndex item) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getItems().predicateForAnyAdjacentBlock(item, predicate);
}
bool HasShapes::isAdjacentToPlantAt(HasShapeIndex index, BlockIndex location, Facing facing, PlantIndex plant) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return occupied.contains(block); };
	return m_area.getPlants().predicateForAnyAdjacentBlock(plant, predicate);
}
DistanceInBlocks HasShapes::distanceToActor(HasShapeIndex index, ActorIndex actor) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_area.getBlocks().distance(m_location.at(index), m_area.getActors().getLocation(actor));
}
bool HasShapes::allOccupiedBlocksAreReservable(HasShapeIndex index, FactionId faction) const
{
	return allBlocksAtLocationAndFacingAreReservable(index, m_location.at(index), m_facing.at(index), faction);
}
bool HasShapes::isAdjacentToLocation(HasShapeIndex index, BlockIndex location) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return block == location; };
	return predicateForAnyAdjacentBlock(index, predicate);
}
bool HasShapes::isAdjacentToAny(HasShapeIndex index, BlockIndices locations) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return locations.contains(block); };
	return predicateForAnyAdjacentBlock(index, predicate);
}
bool HasShapes::isOnEdgeAt(HasShapeIndex index, BlockIndex location, Facing facing) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks](const BlockIndex block) { return blocks.isEdge(block); };
	return predicateForAnyOccupiedBlockAtLocationAndFacing(index, predicate, location, facing);
}
bool HasShapes::isOnEdge(HasShapeIndex index) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks](const BlockIndex block) { return blocks.isEdge(block); };
	return predicateForAnyOccupiedBlock(index, predicate);
}
bool HasShapes::predicateForAnyOccupiedBlock(HasShapeIndex index, std::function<bool(const BlockIndex)> predicate) const
{
	assert(!m_blocks.at(index).empty());
	for(BlockIndex block : m_blocks.at(index))
		if(predicate(block))
			return true;
	return false;
}
bool HasShapes::predicateForAnyOccupiedBlockAtLocationAndFacing(HasShapeIndex index, std::function<bool(const BlockIndex)> predicate, BlockIndex location, Facing facing) const
{
	for(BlockIndex occupied : const_cast<HasShapes&>(*this).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing))
		if(predicate(occupied))
			return true;
	return false;
}
bool HasShapes::predicateForAnyAdjacentBlock(HasShapeIndex index, std::function<bool(const BlockIndex)> predicate) const
{
	assert(m_location.at(index).exists());
	//TODO: cache this.
	for(auto [x, y, z] : m_shape.at(index)->adjacentPositionsWithFacing(m_facing.at(index)))
	{
		BlockIndex block = m_area.getBlocks().offset(m_location.at(index), x, y, z);
		if(block.exists() && predicate(block))
			return true;
	}
	return false;
}
BlockIndices HasShapes::getAdjacentBlocks(HasShapeIndex index) const
{
	BlockIndices output;
	for(BlockIndex block : m_blocks.at(index))
		for(BlockIndex adjacent : m_area.getBlocks().getAdjacentWithEdgeAndCornerAdjacent(block))
			if(!m_blocks.at(index).contains(adjacent))
				output.remove(adjacent);
	return output;
}
ActorIndices HasShapes::getAdjacentActors(HasShapeIndex index) const
{
	ActorIndices output;
	for(BlockIndex block : getBlocks(index))
		output.merge(m_area.getBlocks().actor_getAll(block));
	for(BlockIndex block : getAdjacentBlocks(index))
		output.merge(m_area.getBlocks().actor_getAll(block));
	output.remove(index.toActor());
	return output;
}
ItemIndices HasShapes::getAdjacentItems(HasShapeIndex index) const
{
	ItemIndices output;
	for(BlockIndex block : getOccupiedAndAdjacentBlocks(index))
		output.merge(m_area.getBlocks().item_getAll(block));
	output.remove(index.toItem());
	return output;
}
BlockIndices HasShapes::getBlocksWhichWouldBeOccupiedAtLocationAndFacing(HasShapeIndex index, BlockIndex location, Facing facing) const
{
	return m_shape.at(index)->getBlocksOccupiedAt(m_area.getBlocks(), location, facing);
}
BlockIndices HasShapes::getAdjacentBlocksAtLocationWithFacing(HasShapeIndex index, BlockIndex location, Facing facing) const
{
	BlockIndices output;
	output.reserve(m_shape.at(index)->positions.size());
	for(auto [x, y, z] : m_shape.at(index)->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block.exists())
			output.add(block);
	}
	return output;
}
BlockIndex HasShapes::getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(HasShapeIndex index, const BlockIndex location, Facing facing, std::function<bool(const BlockIndex)>& predicate) const
{
	for(auto [x, y, z] : m_shape.at(index)->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block.exists() && predicate(block))
			return block;
	}
	return BlockIndex::null();
}
BlockIndex HasShapes::getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(HasShapeIndex index, const BlockIndex location, Facing facing, std::function<bool(const BlockIndex)>& predicate) const
{
	for(auto& [x, y, z, v] : m_shape.at(index)->positionsWithFacing(facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block.exists() && predicate(block))
			return block;
	}
	return BlockIndex::null();
}
BlockIndex HasShapes::getBlockWhichIsAdjacentWithPredicate(HasShapeIndex index, std::function<bool(const BlockIndex)>& predicate) const
{
	return getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location.at(index), m_facing.at(index), predicate);
}
BlockIndex HasShapes::getBlockWhichIsOccupiedWithPredicate(HasShapeIndex index, std::function<bool(const BlockIndex)>& predicate) const
{
	return getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(index, m_location.at(index), m_facing.at(index), predicate);
}
ItemIndex HasShapes::getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(HasShapeIndex index, BlockIndex location, Facing facing, std::function<bool(const ItemIndex)>& predicate) const
{
	for(auto [x, y, z] : m_shape.at(index)->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block.exists())
			for(ItemIndex item : m_area.getBlocks().item_getAll(block))
				if(predicate(item))
					return item;
	}
	return ItemIndex::null();
}
ItemIndex HasShapes::getItemWhichIsAdjacentWithPredicate(HasShapeIndex index, std::function<bool(const ItemIndex)>& predicate) const
{
	return getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location.at(index), m_facing.at(index), predicate);
}
bool HasShapes::allBlocksAtLocationAndFacingAreReservable(HasShapeIndex index, const BlockIndex location, Facing facing, FactionId faction) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks, faction](const BlockIndex occupied) { return blocks.isReserved(occupied, faction); };
	return predicateForAnyOccupiedBlockAtLocationAndFacing(index, predicate, location, facing);
}
void HasShapes::log(HasShapeIndex index) const
{
	if(!m_location.at(index).exists())
	{
		std::cout << "*";
		return;
	}
	Point3D coordinates = m_area.getBlocks().getCoordinates(m_location.at(index));
	std::cout << "[" << coordinates.x << "," << coordinates.y << "," << coordinates.z << "]";
}
