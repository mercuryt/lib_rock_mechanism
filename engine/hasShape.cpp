#include "hasShape.h"
#include "block.h"
#include "actor.h"
#include "config.h"
#include "locationBuckets.h"
#include "simulation.h"
#include "types.h"
#include <cassert>
#include <cstdint>
#include <iostream>
HasShape::HasShape(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : 
	m_simulation(deserializationMemo.m_simulation), m_static(data["static"].get<bool>()), m_shape(nullptr),
	m_location(nullptr), m_facing(data["facing"].get<Facing>()),
	m_canLead(*this), m_canFollow(*this), 
	m_reservable(data.contains("quantity") ? data["quantity"].get<uint32_t>() : 1), 
	m_isUnderground(data["isUnderground"].get<bool>()) 
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
	if(m_location)
		data["location"] = m_location;
	if(m_faction)
		data["faction"] = m_faction;
	return data;
}
void HasShape::setShape(const Shape& shape)
{
	Block* location = m_location;
	if(m_location)
	{
		exit();
		if(isActor())
			location->m_locationBucket->erase(static_cast<Actor&>(*this));
	}
	m_shape = &shape;
	if(location)
		setLocation(*location);
}
void HasShape::setStatic(bool isTrue)
{
	assert(m_static != isTrue);
	if(isTrue)
		for(auto& [x, y, z, v] : m_shape->positionsWithFacing(m_facing))
		{
			Block* block = m_location->offset(x, y, z);
			block->m_hasShapes.m_staticVolume += v;
		}
	else
		for(auto& [x, y, z, v] : m_shape->positionsWithFacing(m_facing))
		{
			Block* block = m_location->offset(x, y, z);
			block->m_hasShapes.m_staticVolume -= v;
		}
	m_static = isTrue;
}
void HasShape::reserveOccupied(CanReserve& canReserve)
{
	for(Block* block : m_blocks)
		block->m_reservable.reserveFor(canReserve);
}
bool HasShape::isAdjacentTo(const HasShape& other) const
{
	assert(m_location);
	assert(other.m_location);
	const HasShape* smaller = this;
	const HasShape* larger = &other;
	if(smaller->m_blocks.size () > larger->m_blocks.size())
		std::swap(smaller, larger);
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return larger->m_blocks.contains(const_cast<Block*>(&block)); };
	return smaller->predicateForAnyAdjacentBlock(predicate);
}
DistanceInBlocks HasShape::distanceTo(const HasShape& other) const
{
	// TODO: Make handle multi block creatures correctly somehow.
	// Use line of sight?
	return m_location->distance(*other.m_location);
}
bool HasShape::allOccupiedBlocksAreReservable(Faction& faction) const
{
	return allBlocksAtLocationAndFacingAreReservable(*m_location, m_facing, faction);
}
bool HasShape::isAdjacentToAt(const Block& location, Facing facing, const HasShape& hasShape) const
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return hasShape.m_blocks.contains(const_cast<Block*>(&block)); };
	return const_cast<HasShape*>(this)->getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(location, facing, predicate) != nullptr;
}
bool HasShape::isAdjacentTo(Block& location) const
{
	std::function<bool(const Block&)> predicate = [&](const Block& block) { return block == location; };
	return predicateForAnyAdjacentBlock(predicate);
}
bool HasShape::predicateForAnyOccupiedBlock(std::function<bool(const Block&)> predicate) const
{
	assert(!m_blocks.empty());
	for(Block* block : m_blocks)
		if(predicate(*block))
			return true;
	return false;
}
bool HasShape::predicateForAnyAdjacentBlock(std::function<bool(const Block&)> predicate) const
{
	assert(m_location != nullptr);
	//TODO: cache this.
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(m_facing))
	{
		Block* block = m_location->offset(x, y, z);
		if(block != nullptr && predicate(*block))
			return true;
	}
	return false;
}
std::unordered_set<Block*> HasShape::getAdjacentBlocks()
{
	std::unordered_set<Block*> output;
	for(Block* block : m_blocks)
		for(Block* adjacent : block->getAdjacentWithEdgeAndCornerAdjacent())
			if(!m_blocks.contains(adjacent))
				output.insert(adjacent);
	return output;
}
std::unordered_set<HasShape*> HasShape::getAdjacentHasShapes()
{
	std::unordered_set<HasShape*> output;
	for(Block* block : m_blocks)
	{
		{
			auto& shapes = block->m_hasShapes.getShapes();
			for(auto& pair : shapes)
				output.insert(pair.first);
		}
		for(Block* adjacent : block->m_adjacents)
			if(adjacent)
			{
				auto& shapes = adjacent->m_hasShapes.getShapes();
				for(auto& pair : shapes)
					output.insert(pair.first);
			}
	}
	output.erase(this);
	return output;
}
std::vector<Block*> HasShape::getBlocksWhichWouldBeOccupiedAtLocationAndFacing(Block& location, Facing facing)
{
	return m_shape->getBlocksOccupiedAt(location, facing);
}
std::vector<Block*> HasShape::getAdjacentAtLocationWithFacing(const Block& location, Facing facing)
{
	std::vector<Block*> output;
	output.reserve(m_shape->positions.size());
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(facing))
	{
		Block* block = location.offset(x, y, z);
		if(block != nullptr)
			output.push_back(block);
	}
	return output;
}
Block* HasShape::getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Block&)>& predicate)
{
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(facing))
	{
		Block* block = location.offset(x, y, z);
		if(block != nullptr && predicate(*block))
			return block;
	}
	return nullptr;
}
Block* HasShape::getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Block&)>& predicate)
{
	for(auto& [x, y, z, v] : m_shape->positionsWithFacing(facing))
	{
		Block* block = location.offset(x, y, z);
		if(block != nullptr && predicate(*block))
			return block;
	}
	return nullptr;
}
Block* HasShape::getBlockWhichIsAdjacentWithPredicate(std::function<bool(const Block&)>& predicate)
{
	return getBlockWhichIsAdjacentAtLocationWithFacingAndPredicate(*m_location, m_facing, predicate);
}
Block* HasShape::getBlockWhichIsOccupiedWithPredicate(std::function<bool(const Block&)>& predicate)
{
	return getBlockWhichIsOccupiedAtLocationWithFacingAndPredicate(*m_location, m_facing, predicate);
}
Item* HasShape::getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(const Block& location, Facing facing, std::function<bool(const Item&)>& predicate)
{
	for(auto [x, y, z] : m_shape->adjacentPositionsWithFacing(facing))
	{
		Block* block = location.offset(x, y, z);
		if(block != nullptr)
			for(Item* item : block->m_hasItems.getAll())
				if(predicate(*item))
					return item;
	}
	return nullptr;
}
Item* HasShape::getItemWhichIsAdjacentWithPredicate(std::function<bool(const Item&)>& predicate)
{
	return getItemWhichIsAdjacentAtLocationWithFacingAndPredicate(*m_location, m_facing, predicate);
}
bool HasShape::allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing, Faction& faction) const
{
	for(Block* occupied : const_cast<HasShape&>(*this).getBlocksWhichWouldBeOccupiedAtLocationAndFacing(const_cast<Block&>(location), facing))
		if(occupied->m_reservable.isFullyReserved(&faction))
			return false;
	return true;
}
EventSchedule& HasShape::getEventSchedule() { return getSimulation().m_eventSchedule; }
void HasShape::log() const
{
	if(m_location == nullptr)
	{
		std::cout << "*";
		return;
	}
	std::cout << "[" << m_location->m_x << "," << m_location->m_y << "," << m_location->m_z << "]";
}
