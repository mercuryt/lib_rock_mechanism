#include "hasShape.h"
#include "area.h"
#include "actor.h"
#include "config.h"
#include "locationBuckets.h"
#include "simulation.h"
#include "types.h"
#include <cassert>
#include <cstdint>
#include <iostream>
HasShape::HasShape(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : 
	m_reservable(data.contains("quantity") ? data["quantity"].get<uint32_t>() : 1), m_canLead(*this), m_canFollow(*this, deserializationMemo.m_simulation),
	m_simulation(deserializationMemo.m_simulation), m_shape(nullptr),
	m_facing(data["facing"].get<Facing>()), 
	m_isUnderground(data["isUnderground"].get<bool>()), 
	m_static(data["static"].get<bool>()) 
{ 
	// data["location"] is not read here because we want to use subclass specific setLocation methods.
	if(data.contains("reservable"))
		deserializationMemo.m_reservables[data["reservable"].get<uintptr_t>()] = &m_reservable;
	if(data.contains("faction"))
		m_faction = &deserializationMemo.faction(data["faction"].get<std::wstring>());
	std::string shapeName = data["shape"].get<std::string>();
	if(Shape::hasShape(shapeName))
		// Predefined shape.
		m_shape = &Shape::byName(shapeName);
	else
	{
		// Custom shape.
		if(!m_simulation.m_shapes.contains(shapeName))
			m_simulation.m_shapes.loadFromName(shapeName);
		m_shape = &m_simulation.m_shapes.byName(shapeName);
	}
}
Json HasShape::toJson() const
{
	Json data{{"static", m_static}, {"shape", m_shape}, {"canFollow", m_canFollow.toJson()}, {"facing", m_facing}, 
		{"isUnderground", m_isUnderground}, 
		{"address", reinterpret_cast<uintptr_t>(this)}};
	if(m_reservable.hasAnyReservations())
		data["reservable"] = reinterpret_cast<uintptr_t>(&m_reservable);
	if(m_location != BLOCK_INDEX_MAX)
		data["location"] = m_location;
	if(m_faction)
		data["faction"] = m_faction;
	return data;
}
void HasShape::setShape(const Shape& shape)
{
	BlockIndex location = m_location;
	if(m_location != BLOCK_INDEX_MAX)
	{
		exit();
		if(isActor())
			m_area->getBlocks().getLocationBucket(location).erase(static_cast<Actor&>(*this));
	}
	m_shape = &shape;
	if(location != BLOCK_INDEX_MAX)
		setLocation(location, m_area);
}
void HasShape::setStatic(bool isTrue)
{
	assert(m_static != isTrue);
	if(isTrue)
		for(auto& [x, y, z, v] : m_shape->positionsWithFacing(m_facing))
		{
			BlockIndex block = m_area->getBlocks().offset(m_location, x, y, z);
			m_area->getBlocks().shape_addStaticVolume(block, v);
		}
	else
		for(auto& [x, y, z, v] : m_shape->positionsWithFacing(m_facing))
		{
			BlockIndex block = m_area->getBlocks().offset(m_location, x, y, z);
			m_area->getBlocks().shape_removeStaticVolume(block, v);
		}
	m_static = isTrue;
}
void HasShape::reserveOccupied(CanReserve& canReserve)
{
	for(BlockIndex block : m_blocks)
		m_area->getBlocks().reserve(block, canReserve);
}
bool HasShape::isAdjacentTo(const HasShape& other) const
{
	assert(m_location != BLOCK_INDEX_MAX);
	assert(other.m_location != BLOCK_INDEX_MAX);
	const HasShape* smaller = this;
	const HasShape* larger = &other;
	if(smaller->m_blocks.size () > larger->m_blocks.size())
		std::swap(smaller, larger);
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block) { return larger->m_blocks.contains(block); };
	return smaller->predicateForAnyAdjacentBlock(predicate);
}
DistanceInBlocks HasShape::distanceTo(const HasShape& other) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_area->getBlocks().distance(m_location, other.m_location);
}
bool HasShape::allOccupiedBlocksAreReservable(Faction& faction) const
{
	return allBlocksAtLocationAndFacingAreReservable(m_location, m_facing, faction);
}
bool HasShape::isAdjacentToAt(const BlockIndex location, Facing facing, const HasShape& hasShape) const
{
	std::function<bool(const BlockIndex)> predicate = [&](BlockIndex block) { return hasShape.m_blocks.contains(block); };
	return const_cast<HasShape*>(this)->getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate) != BLOCK_INDEX_MAX;
}
bool HasShape::isAdjacentTo(BlockIndex location) const
{
	std::function<bool(const BlockIndex)> predicate = [&](const BlockIndex block) { return block == location; };
	return predicateForAnyAdjacentBlock(predicate);
}
bool HasShape::predicateForAnyOccupiedBlock(std::function<bool(const BlockIndex)> predicate) const
{
	assert(!m_blocks.empty());
	for(BlockIndex block : m_blocks)
		if(predicate(block))
			return true;
	return false;
}
bool HasShape::predicateForAnyAdjacentBlock(std::function<bool(const BlockIndex)> predicate) const
{
	assert(m_location != BLOCK_INDEX_MAX);
	//TODO: cache this.
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(m_facing))
	{
		BlockIndex block = m_area->getBlocks().offset(m_location, x, y, z);
		if(block != BLOCK_INDEX_MAX && predicate(block))
			return true;
	}
	return false;
}
std::unordered_set<BlockIndex> HasShape::getAdjacentBlocks()
{
	std::unordered_set<BlockIndex> output;
	for(BlockIndex block : m_blocks)
		for(BlockIndex adjacent : m_area->getBlocks().getAdjacentWithEdgeAndCornerAdjacent(block))
			if(!m_blocks.contains(adjacent))
				output.insert(adjacent);
	return output;
}
const std::unordered_set<BlockIndex> HasShape::getAdjacentBlocks() const
{
	return const_cast<HasShape*>(this)->getAdjacentBlocks();
}
std::unordered_set<HasShape*> HasShape::getAdjacentHasShapes()
{
	std::unordered_set<HasShape*> output;
	for(BlockIndex block : m_blocks)
	{
		{
			auto& shapes = m_area->getBlocks().shape_getShapes(block);
			for(auto& pair : shapes)
				output.insert(pair.first);
		}
		for(BlockIndex adjacent : m_area->getBlocks().getDirectlyAdjacent(block))
			if(adjacent != BLOCK_INDEX_MAX)
			{
				auto& shapes = m_area->getBlocks().shape_getShapes(adjacent);
				for(auto& pair : shapes)
					output.insert(pair.first);
			}
	}
	output.erase(this);
	return output;
}
std::vector<BlockIndex> HasShape::getBlocksWhichWouldBeOccupiedAtLocationAndFacing(BlockIndex location, Facing facing)
{
	return m_shape->getBlocksOccupiedAt(m_area->getBlocks(), location, facing);
}
std::vector<BlockIndex> HasShape::getAdjacentAtLocationWithFacing(const BlockIndex location, Facing facing)
{
	std::vector<BlockIndex> output;
	output.reserve(m_shape->positions.size());
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area->getBlocks().offset(location, x, y, z);
		if(block != BLOCK_INDEX_MAX)
			output.push_back(block);
	}
	return output;
}
BlockIndex HasShape::getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const BlockIndex location, Facing facing, std::function<bool(const BlockIndex)>& predicate)
{
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area->getBlocks().offset(location, x, y, z);
		if(block != BLOCK_INDEX_MAX && predicate(block))
			return block;
	}
	return BLOCK_INDEX_MAX;
}
BlockIndex HasShape::getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const BlockIndex location, Facing facing, std::function<bool(const BlockIndex)>& predicate)
{
	for(auto& [x, y, z, v] : m_shape->positionsWithFacing(facing))
	{
		BlockIndex block = m_area->getBlocks().offset(location, x, y, z);
		if(block != BLOCK_INDEX_MAX && predicate(block))
			return block;
	}
	return BLOCK_INDEX_MAX;
}
BlockIndex HasShape::getBlockWhichIsAdjacentWithPredicate(std::function<bool(const BlockIndex)>& predicate)
{
	return getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(m_location, m_facing, predicate);
}
BlockIndex HasShape::getBlockWhichIsOccupiedWithPredicate(std::function<bool(const BlockIndex)>& predicate)
{
	return getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(m_location, m_facing, predicate);
}
Item* HasShape::getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const BlockIndex location, Facing facing, std::function<bool(const Item&)>& predicate)
{
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(facing))
	{
		BlockIndex block = m_area->getBlocks().offset(location, x, y, z);
		if(block != BLOCK_INDEX_MAX)
			for(Item* item : m_area->getBlocks().item_getAll(block))
				if(predicate(*item))
					return item;
	}
	return nullptr;
}
Item* HasShape::getItemWhichIsAdjacentWithPredicate(std::function<bool(const Item&)>& predicate)
{
	return getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(m_location, m_facing, predicate);
}
bool HasShape::allBlocksAtLocationAndFacingAreReservable(const BlockIndex location, Facing facing, Faction& faction) const
{
	for(BlockIndex occupied : const_cast<HasShape&>(*this).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(location, facing))
		if(m_area->getBlocks().isReserved(occupied, faction))
			return false;
	return true;
}
EventSchedule& HasShape::getEventSchedule() { return getSimulation().m_eventSchedule; }
void HasShape::log() const
{
	if(m_location == BLOCK_INDEX_MAX)
	{
		std::cout << "*";
		return;
	}
	Point3D coordinates = m_area->getBlocks().getCoordinates(m_location);
	std::cout << "[" << coordinates.x << "," << coordinates.y << "," << coordinates.z << "]";
}
