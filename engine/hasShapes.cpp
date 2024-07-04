#include "hasShapes.h"
#include "actors/actors.h"
#include "area.h"
#include "blocks/blocks.h"
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
	if(m_freeSlots.empty())
	{
		HasShapeIndex index =  m_shape.size();
		resize(index);
		return index;
	}
	else
	{
		auto index = m_freeSlots.back();
		m_freeSlots.pop_back();
		return index;
	}
}
void HasShapes::create(HasShapeIndex index, const Shape& shape, BlockIndex location, Facing facing, bool isStatic)
{
	assert(m_shape.size() > index);
	m_shape.at(index) = &shape;
	m_location.at(index) = location;
	m_facing.at(index) = facing;
	m_static[index] = isStatic;
}
void HasShapes::resize(HasShapeIndex newSize)
{
	m_shape.resize(newSize);
	m_location.resize(newSize);
	m_facing.resize(newSize);
	m_blocks.resize(newSize);
	m_static.resize(newSize);
	m_underground.reserve(newSize);
}
void HasShapes::moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex)
{
	m_shape.at(newIndex) = m_shape.at(oldIndex);
	m_location.at(newIndex) = m_location.at(oldIndex);
	m_facing.at(newIndex) = m_facing.at(oldIndex);
	m_blocks.at(newIndex) = m_blocks.at(oldIndex);
	m_static[newIndex] = m_static[oldIndex];
	m_underground[newIndex] = m_underground[oldIndex];
}
void HasShapes::destroy(HasShapeIndex index)
{
	// If the index is the last one resize.
	if(index == m_shape.size() - 1)
		resize(index);
	/*
	//TODO: write moveIndex and then do these comments.
	// if the last index can be moved move it to this index and then resize.
	else if(indexCanBeMoved(m_shape.size() - 1))
	{
		moveIndex(m_shape.size() - 1, index);
		resize(index);
	}
	// try to find the highest index which can be moved, then mark the old high index as free.
	// No higher index to swap with, mark this one as free.
	// */
	m_freeSlots.push_back(index);
}
void HasShapes::setShape(HasShapeIndex index, const Shape& shape)
{
	BlockIndex location = m_location.at(index);
	if(location != BLOCK_INDEX_MAX)
	{
		exit(index);
		//TODO: virtual function?
		//if(isActor())
			//m_area->getBlocks().getLocationBucket(location).erase(static_cast<Actor&>(*this));
	}
	m_shape.at(index) = &shape;
	if(location != BLOCK_INDEX_MAX)
		setLocation(index, location);
}
void HasShapes::setStatic(HasShapeIndex index, bool isTrue)
{
	assert(m_static[index] != isTrue);
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
	m_static[index] = isTrue;
}
void HasShapes::updateIsOnSurface(HasShapeIndex index, BlockIndex block)
{
	assert(block == getLocation(index));

	if(m_area.getBlocks().isOnSurface(block))
		m_onSurface.insert(index);
	else
		m_onSurface.erase(index);
}
void HasShapes::sortRange(HasShapeIndex begin, HasShapeIndex end)
{
	assert(end > begin);
	//TODO: impliment sort with library rather then views::zip for clang / msvc support.
	/*
	auto zip = std::ranges::views::zip(m_shape, m_location, m_facing, m_faction, m_blocks, m_static, m_underground);
	std::ranges::sort(zip.begin() + begin, zip.end() + end, SpatialSort);
	*/
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
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return util::vectorContains(occupied, block); };
	return m_area.getActors().predicateForAnyAdjacentBlock(actor, predicate);
}
bool HasShapes::isAdjacentToItemAt(HasShapeIndex index, BlockIndex location, Facing facing, ItemIndex item) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return util::vectorContains(occupied, block); };
	return m_area.getItems().predicateForAnyAdjacentBlock(item, predicate);
}
bool HasShapes::isAdjacentToPlantAt(HasShapeIndex index, BlockIndex location, Facing facing, PlantIndex plant) const
{
	auto occupied = getBlocksWhichWouldBeOccupiedAtLocationAndFacing(index, location, facing);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return util::vectorContains(occupied, block); };
	return m_area.getPlants().predicateForAnyAdjacentBlock(plant, predicate);
}
DistanceInBlocks HasShapes::distanceToActor(HasShapeIndex index, ActorIndex actor) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_area.getBlocks().distance(m_location.at(index), m_area.getActors().getLocation(actor));
}
bool HasShapes::allOccupiedBlocksAreReservable(HasShapeIndex index, Faction& faction) const
{
	return allBlocksAtLocationAndFacingAreReservable(index, m_location.at(index), m_facing.at(index), faction);
}
bool HasShapes::isAdjacentToLocation(HasShapeIndex index, BlockIndex location) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return block == location; };
	return predicateForAnyAdjacentBlock(index, predicate);
}
bool HasShapes::isAdjacentToAny(HasShapeIndex index, std::unordered_set<BlockIndex> locations) const
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
	assert(m_location.at(index) != BLOCK_INDEX_MAX);
	//TODO: cache this.
	for(auto [x, y, z] : m_shape.at(index)->adjacentPositionsWithFacing(m_facing.at(index)))
	{
		BlockIndex block = m_area.getBlocks().offset(m_location.at(index), x, y, z);
		if(block != BLOCK_INDEX_MAX && predicate(block))
			return true;
	}
	return false;
}
std::unordered_set<BlockIndex> HasShapes::getAdjacentBlocks(HasShapeIndex index) const
{
	std::unordered_set<BlockIndex> output;
	for(BlockIndex block : m_blocks.at(index))
		for(BlockIndex adjacent : m_area.getBlocks().getAdjacentWithEdgeAndCornerAdjacent(block))
			if(!m_blocks.at(index).contains(adjacent))
				output.insert(adjacent);
	return output;
}
std::unordered_set<ActorIndex> HasShapes::getAdjacentActors(HasShapeIndex index) const
{
	std::unordered_set<ActorIndex> output;
	for(BlockIndex block : getBlocks(index))
	{
		auto& actors = m_area.getBlocks().actor_getAll(block);
		output.insert(actors.begin(), actors.end());
	}
	for(BlockIndex block : getAdjacentBlocks(index))
	{
		auto& actors = m_area.getBlocks().actor_getAll(block);
		output.insert(actors.begin(), actors.end());
	}
	output.erase(index);
	return output;
}
std::unordered_set<ItemIndex> HasShapes::getAdjacentItems(HasShapeIndex index) const
{
	std::unordered_set<ItemIndex> output;
	for(BlockIndex block : getOccupiedAndAdjacentBlocks(index))
	{
		auto items = m_area.getBlocks().item_getAll(block);
		output.insert(items.begin(), items.end());
	}
	output.erase(index);
	return output;
}
std::vector<BlockIndex> HasShapes::getBlocksWhichWouldBeOccupiedAtLocationAndFacing(HasShapeIndex index, BlockIndex location, Facing facing) const
{
	return m_shape.at(index)->getBlocksOccupiedAt(m_area.getBlocks(), location, facing);
}
std::vector<BlockIndex> HasShapes::getAdjacentBlocksAtLocationWithFacing(HasShapeIndex index, BlockIndex location, Facing facing) const
{
	std::vector<BlockIndex> output;
	output.reserve(m_shape.at(index)->positions.size());
	for(auto [x, y, z] : m_shape.at(index)->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
BlockIndex HasShapes::getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(HasShapeIndex index, const BlockIndex location, Facing facing, std::function<bool(const BlockIndex)>& predicate) const
{
	for(auto [x, y, z] : m_shape.at(index)->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block != BLOCK_INDEX_MAX && predicate(block))
			return block;
	}
	return BLOCK_INDEX_MAX;
}
BlockIndex HasShapes::getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(HasShapeIndex index, const BlockIndex location, Facing facing, std::function<bool(const BlockIndex)>& predicate) const
{
	for(auto& [x, y, z, v] : m_shape.at(index)->positionsWithFacing(facing))
	{
		BlockIndex block = m_area.getBlocks().offset(location, x, y, z);
		if(block != BLOCK_INDEX_MAX && predicate(block))
			return block;
	}
	return BLOCK_INDEX_MAX;
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
		if(block != BLOCK_INDEX_MAX)
			for(ItemIndex item : m_area.getBlocks().item_getAll(block))
				if(predicate(item))
					return item;
	}
	return ITEM_INDEX_MAX;
}
ItemIndex HasShapes::getItemWhichIsAdjacentWithPredicate(HasShapeIndex index, std::function<bool(const ItemIndex)>& predicate) const
{
	return getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(index, m_location.at(index), m_facing.at(index), predicate);
}
bool HasShapes::allBlocksAtLocationAndFacingAreReservable(HasShapeIndex index, const BlockIndex location, Facing facing, Faction& faction) const
{
	Blocks& blocks = m_area.getBlocks();
	std::function<bool(const BlockIndex)> predicate = [&blocks, &faction](const BlockIndex occupied) { return blocks.isReserved(occupied, faction); };
	return predicateForAnyOccupiedBlockAtLocationAndFacing(index, predicate, location, facing);
}
void HasShapes::log(HasShapeIndex index) const
{
	if(m_location.at(index) == BLOCK_INDEX_MAX)
	{
		std::cout << "*";
		return;
	}
	Point3D coordinates = m_area.getBlocks().getCoordinates(m_location.at(index));
	std::cout << "[" << coordinates.x << "," << coordinates.y << "," << coordinates.z << "]";
}
const Shape& HasShapes::getShape(HasShapeIndex index) const { return *m_shape.at(index); }
BlockIndex HasShapes::getLocation(HasShapeIndex index)  const { return m_location.at(index); }
Facing HasShapes::getFacing(HasShapeIndex index) const { return m_facing.at(index); }
std::vector<HasShapeIndex> HasShapes::getAll() const
{
	//TODO: cache?
	std::vector<HasShapeIndex> output;
	output.reserve(m_shape.size());
	for(HasShapeIndex i = 0; i < m_shape.size(); ++i)
		if(std::ranges::find(m_freeSlots, i) == m_freeSlots.end())
			output.push_back(i);
	return output;
}
