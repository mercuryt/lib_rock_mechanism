#include "hasShape.h"
#include "block.h"
#include "config.h"
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
	return data;
}
void HasShape::setShape(const Shape& shape)
{
	Block* location = m_location;
	if(m_location)
		exit();
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
bool HasShape::allOccupiedBlocksAreReservable(const Faction& faction) const
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
bool HasShape::allBlocksAtLocationAndFacingAreReservable(const Block& location, Facing facing, const Faction& faction) const
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
void BlockHasShapes::record(HasShape& hasShape, CollisionVolume volume)
{
	assert(!m_shapes.contains(&hasShape));
	m_shapes[&hasShape] = volume;
	m_totalVolume += volume;
	// Do not assert that maxBlockVolume has not been exceeded, volumes in excess of max are allowed in some circumstances.
	if(hasShape.m_static)
	{
		m_staticVolume += volume;
		clearCache();
	}
}
void BlockHasShapes::remove(HasShape& hasShape)
{
	assert(m_shapes.contains(&hasShape));
	auto v = m_shapes.at(&hasShape);
	m_totalVolume -= v;
	m_shapes.erase(&hasShape);
	if(hasShape.m_static)
	{
		m_staticVolume -= v;
		clearCache();
	}
}
void BlockHasShapes::enter(HasShape& hasShape)
{
	assert(hasShape.m_location != &m_block);
	assert(hasShape.m_blocks.empty());
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(hasShape.m_facing))
	{
		Block* block = m_block.offset(x, y, z);
		assert(block);
		block->m_hasShapes.record(hasShape, v);
		hasShape.m_blocks.insert(block);
	}
	hasShape.m_location = &m_block;
}
void BlockHasShapes::exit(HasShape& hasShape)
{
	assert(hasShape.m_location == &m_block);
	for(Block* block : hasShape.m_blocks)
		block->m_hasShapes.remove(hasShape);
	hasShape.m_blocks.clear();
	hasShape.m_location = nullptr;
}
//TODO: Move this to item.
void BlockHasShapes::addQuantity(HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	exit(hasShape);
	item.addQuantity(quantity);
	enter(hasShape);
}
void BlockHasShapes::removeQuantity(HasShape& hasShape, uint32_t quantity)
{
	assert(hasShape.isItem());
	Item& item = static_cast<Item&>(hasShape);
	assert(item.m_itemType.generic);
	exit(hasShape);
	item.removeQuantity(quantity);
	enter(hasShape);
}
void BlockHasShapes::tryToCacheMoveCosts(const Shape& shape, const MoveType& moveType, std::vector<std::pair<Block*, MoveCost>>& moveCosts)
{
	if(!m_moveCostsCache.contains(&shape) || !m_moveCostsCache.at(&shape).contains(&moveType))
		m_moveCostsCache[&shape][&moveType] = std::move(moveCosts);
}
void BlockHasShapes::clearCache()
{
	m_moveCostsCache.clear();
	for(Block* block : m_block.getAdjacentWithEdgeAndCornerAdjacent())
		block->m_hasShapes.m_moveCostsCache.clear();
}
bool BlockHasShapes::anythingCanEnterEver() const
{
	if(m_block.isSolid())
		return false;
	// TODO: cache this.
	return !m_block.m_hasBlockFeatures.blocksEntrance();
}
bool BlockHasShapes::canEnterEverFrom(const HasShape& hasShape, const Block& block) const
{
	return shapeAndMoveTypeCanEnterEverFrom(*hasShape.m_shape, hasShape.getMoveType(), block);
}
bool BlockHasShapes::canEnterEverWithFacing(const HasShape& hasShape, const Facing facing) const
{
	return shapeAndMoveTypeCanEnterEverWithFacing(*hasShape.m_shape, hasShape.getMoveType(), facing);
}
bool BlockHasShapes::canEnterEverWithAnyFacing(const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(canEnterEverWithFacing(hasShape, facing))
			return true;
	return false;
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterEverFrom(const Shape& shape, const MoveType& moveType, const Block& from) const
{
	assert(from.m_hasShapes.anythingCanEnterEver());
	if(!anythingCanEnterEver())
		return false;
	if(!moveTypeCanEnterFrom(moveType, from))
		return false;
	const Facing facing = m_block.facingToSetWhenEnteringFrom(from);
	return shapeAndMoveTypeCanEnterEverWithFacing(shape, moveType, facing);
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterEverWithFacing(const Shape& shape, const MoveType& moveType, const Facing facing) const
{
	if(!anythingCanEnterEver())
		return false;
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		if(block == nullptr || block->isSolid() || 
				block->m_hasShapes.m_staticVolume + v > Config::maxBlockVolumeHardLimit || 
				!block->m_hasShapes.moveTypeCanEnter(moveType))
			return false;
	}
	return true;
}
bool BlockHasShapes::shapeAndMoveTypeCanEnterCurrentlyWithFacing(const Shape& shape, const MoveType& moveType, const Facing facing) const
{
	assert(anythingCanEnterEver());
	for(auto& [x, y, z, v] : shape.positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		assert(block);
		assert(!block->isSolid());
		if( block->m_hasShapes.m_totalVolume + v > Config::maxBlockVolumeHardLimit || 
			!block->m_hasShapes.moveTypeCanEnter(moveType)
		)
			return false;
	}
	return true;
}
bool BlockHasShapes::moveTypeCanEnterFrom(const MoveType& moveType, const Block& from) const
{
	for(auto& [fluidType, volume] : moveType.swim)
	{
		// Can travel within and enter liquid from any angle.
		if(m_block.volumeOfFluidTypeContains(*fluidType) >= volume)
			return true;
		// Can leave liquid at any angle.
		if(from.volumeOfFluidTypeContains(*fluidType) >= volume)
			return true;
	}
	// Can always enter on same z level.
	if(m_block.m_z == from.m_z)
		return true;
	// Cannot go up if:
	if(m_block.m_z > from.m_z && !m_block.m_hasBlockFeatures.canEnterFromBelow())
		return false;
	// Cannot go down if:
	if(m_block.m_z < from.m_z && !m_block.m_hasBlockFeatures.canEnterFromAbove(from))
		return false;
	// Can enter from any angle if flying or climbing.
	if(moveType.fly || moveType.climb > 0)
		return true;
	// Can go up if from contains a ramp or stairs.
	if(m_block.m_z > from.m_z && (from.m_hasBlockFeatures.contains(BlockFeatureType::ramp) || from.m_hasBlockFeatures.contains(BlockFeatureType::stairs))) 
		return true;
	// Can go down if this contains a ramp or stairs.
	if(m_block.m_z < from.m_z && (m_block.m_hasBlockFeatures.contains(BlockFeatureType::ramp) || 
					m_block.m_hasBlockFeatures.contains(BlockFeatureType::stairs)
					))
		return true;
	return false;
}
bool BlockHasShapes::moveTypeCanEnter(const MoveType& moveType) const
{
	// Swiming.
	for(auto& [fluidType, pair] : m_block.m_fluids)
	{
		auto found = moveType.swim.find(fluidType);
		if(found != moveType.swim.end() && found->second <= pair.first)
		{
			// Can swim in fluid, check breathing requirements
			if(moveType.breathless)
				return true;
			if(moveTypeCanBreath(moveType))
				return true;
			Block* above = m_block.getBlockAbove();
			if(above && !above->isSolid() && above->m_hasShapes.moveTypeCanBreath(moveType))
				return true;
		}
	}
	// Not swimming and fluid level is too high.
	if(m_block.m_totalFluidVolume > Config::maxBlockVolume / 2)
		return false;
	// Fly can always enter if fluid level isn't preventing it.
	if(moveType.fly)
		return true;
	// Walk can enter only if can stand in or if also has climb2 and there is a isSupport() block adjacent.
	if(moveType.walk)
	{
		if(canStandIn())
			return true;
		else

			if(moveType.climb < 2)
				return false;
			else
			{
				// Only climb2 moveTypes can enter.
				for(Block* block : m_block.getAdjacentOnSameZLevelOnly())
					//TODO: check for climable features?
					if(block->isSupport())
						return true;
				return false;
			}
	}
	return false;
}
bool BlockHasShapes::moveTypeCanBreath(const MoveType& moveType) const
{
	if(m_block.m_totalFluidVolume < Config::maxBlockVolume && !moveType.onlyBreathsFluids)
		return true;
	for(auto& pair : m_block.m_fluids)
		//TODO: Minimum volume should probably be scaled by body size somehow.
		if(moveType.breathableFluids.contains(pair.first) && pair.second.first >= Config::minimumVolumeOfFluidToBreath)
			return true;
	return false;
}
bool BlockHasShapes::hasCachedMoveCosts(const Shape& shape, const MoveType& moveType) const
{
	return m_moveCostsCache.contains(&shape) && m_moveCostsCache.at(&shape).contains(&moveType);
}
const std::vector<std::pair<Block*, MoveCost>>& BlockHasShapes::getCachedMoveCosts(const Shape& shape, const MoveType& moveType) const
{
	assert(hasCachedMoveCosts(shape, moveType));
	return m_moveCostsCache.at(&shape).at(&moveType);
}
const std::vector<std::pair<Block*, MoveCost>> BlockHasShapes::makeMoveCosts(const Shape& shape, const MoveType& moveType) const
{
	std::vector<std::pair<Block*, MoveCost>> output;
	for(Block* block : m_block.getAdjacentWithEdgeAndCornerAdjacent())
		if(block->m_hasShapes.shapeAndMoveTypeCanEnterEverFrom(shape, moveType, m_block))
			output.emplace_back(block, block->m_hasShapes.moveCostFrom(moveType, m_block));
	return output;
}
MoveCost BlockHasShapes::moveCostFrom(const MoveType& moveType, const Block& from) const
{
	assert(!m_block.isSolid());
	assert(!from.isSolid());
	if(moveType.fly)
		return Config::baseMoveCost;
	for(auto& [fluidType, volume] : moveType.swim)
		if(m_block.volumeOfFluidTypeContains(*fluidType) >= volume)
			return Config::baseMoveCost;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(m_block.m_z > from.m_z && !from.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		return Config::goUpMoveCost;
	return Config::baseMoveCost;
}
bool BlockHasShapes::canEnterCurrentlyFrom(const HasShape& hasShape, const Block& block) const
{
	const Facing facing = m_block.facingToSetWhenEnteringFrom(block);
	return canEnterCurrentlyWithFacing(hasShape, facing);
}
bool BlockHasShapes::canEnterCurrentlyWithFacing(const HasShape& hasShape, const Facing& facing) const
{
	for(auto& [x, y, z, v] : hasShape.m_shape->positionsWithFacing(facing))
	{
		Block* block = m_block.offset(x, y, z);
		assert(block != nullptr);
		if(block->m_hasShapes.m_totalVolume + v - block->m_hasShapes.getVolume(hasShape) > Config::maxBlockVolumeHardLimit)
			return false;
	}
	return true;
}
bool BlockHasShapes::canEnterCurrentlyWithAnyFacing(const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(canEnterCurrentlyWithFacing(hasShape, facing))
			return true;
	return false;
}
std::pair<bool, Facing> BlockHasShapes::canEnterCurrentlyWithAnyFacingReturnFacing(const HasShape& hasShape) const
{
	for(Facing facing = 0; facing < 4; ++facing)
		if(canEnterCurrentlyWithFacing(hasShape, facing))
			return {true, facing};
	return {false, 0};
}
CollisionVolume BlockHasShapes::getVolume(const HasShape& hasShape) const
{	
	auto found = m_shapes.find(const_cast<HasShape*>(&hasShape));
	if(found == m_shapes.end())
		return 0u;
	return found->second;
}
CollisionVolume BlockHasShapes::getTotalVolume() const 
{
	return m_totalVolume;
}
uint32_t BlockHasShapes::getQuantityOfItemWhichCouldFit(const ItemType& itemType) const
{
	if(m_totalVolume > Config::maxBlockVolume)
		return 0;
	CollisionVolume freeVolume = Config::maxBlockVolume - m_staticVolume;
	return freeVolume / itemType.shape.getCollisionVolumeAtLocationBlock();
}
bool BlockHasShapes::canStandIn() const
{
	return (m_block.m_adjacents[0] != nullptr && (m_block.m_adjacents[0]->isSolid() || 
				m_block.m_adjacents[0]->m_hasBlockFeatures.canStandAbove())) || 
		m_block.m_hasBlockFeatures.canStandIn();
}
