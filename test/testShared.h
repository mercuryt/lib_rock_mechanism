/*
 * Example config used for testing. Attempts to replicate Dwarf Fortress mechanics regarding vision, pathing, cave in, and fluids; except max fluid volume is 100 rather then 7.
 */
#include <cstdint>
#include <algorithm>
#include "../lib/BS_thread_pool_light.hpp"
namespace Config
{
	constexpr uint32_t maxBlockVolume = 100;
	constexpr uint32_t actorDoVisionInterval = 10;
	constexpr float pathHuristicConstant = 1;
	constexpr float maxDistanceVisionModifier = 1.1;
	constexpr uint32_t locationBucketSize = 25;
	constexpr bool fluidPiston = true;
	constexpr uint32_t visionThreadingBatchSize = 30;
	constexpr uint32_t routeThreadingBatchSize = 10;
	// How many units seep through each step max = viscosity / seepDiagonalModifier.
	// Disable by setting to 0.
	constexpr uint32_t fluidsSeepDiagonalModifier = 100;
	constexpr uint32_t moveTryAttemptsBeforeDetour = 2;
}

#include "../src/block.h"
#include "../src/actor.h"
#include "../src/area.h"

#include "../src/moveType.h"
#include "../src/fluidType.h"
#include "../src/materialType.h"
#include "../src/blockFeatureType.h"

inline uint32_t s_step;
BS::thread_pool_light s_pool;

const static MoveType* s_twoLegs;
const static MoveType* s_fourLegs;
const static MoveType* s_flying;
const static MoveType* s_swimmingInWater;
const static MoveType* s_twoLegsAndSwimmingInWater;
const static MoveType* s_fourLegsAndSwimmingInWater;
const static MoveType* s_swimmingInMercury;
const static MoveType* s_twoLegsAndSwimmingInMercury;
const static MoveType* s_twoLegsAndClimb1;
const static MoveType* s_twoLegsAndClimb2;
const static FluidType* s_water;
const static FluidType* s_CO2;
const static FluidType* s_lava;
const static FluidType* s_mercury;
const static MaterialType* s_stone;
const static MaterialType* s_glass;
const static Shape* s_oneByOneFull;
const static Shape* s_oneByOneHalfFull;
const static Shape* s_oneByOneQuarterFull;
const static Shape* s_twoByTwoFull;
const static BlockFeatureType* s_upStairs;
const static BlockFeatureType* s_downStairs;
const static BlockFeatureType* s_upDownStairs;
const static BlockFeatureType* s_ramp;
const static BlockFeatureType* s_fortification;
const static BlockFeatureType* s_door;
const static BlockFeatureType* s_hatch;
const static BlockFeatureType* s_floor;
const static BlockFeatureType* s_floodGate;
const static BlockFeatureType* s_floorGrate;


struct BlockFeature 
{
	const BlockFeatureType* blockFeatureType;
	const MaterialType* materialType;
	bool hewn;
	bool closed;
	bool locked;
	BlockFeature(const BlockFeatureType* bft, const MaterialType* mt, bool h) : blockFeatureType(bft), materialType(mt), hewn(h), closed(true), locked(false) {}
};

class Actor;
class Block;
class Area;

// Put custom member data declarations here
class Block final : public BaseBlock<Block, Actor, Area>
{
public:
	std::vector<BlockFeature> m_features;

	bool anyoneCanEnterEver() const;
	bool moveTypeCanEnter(const MoveType* moveType) const;
	bool moveTypeCanEnterFrom(const MoveType* moveType, Block* from) const;
	uint32_t moveCost(const MoveType* moveType, Block* origin) const;
	std::vector<std::pair<Block*, uint32_t>> getMoveCosts(const Shape* shape, const MoveType* moveType);
	bool canStandIn() const;
	void clearMoveCostsCacheForSelfAndAdjacent();

	bool canSeeIntoFromAlways(const Block* block) const;
	bool canSeeThrough() const;
	bool canSeeThroughFrom(const Block& block) const;
	float visionDistanceModifier() const;

	bool fluidCanEnterEver() const;
	bool fluidCanEnterEver(const FluidType* fluidType) const;

	bool isSupport() const;
	uint32_t getMass() const;

	void moveContentsTo(Block* block);

	void addConstructedFeature(const BlockFeatureType* blockFeatureType, const MaterialType* materialType);
	void addHewnFeature(const BlockFeatureType* blockFeatureType, const MaterialType* materialType = nullptr);
	bool hasFeatureType(const BlockFeatureType* blockFeatureType) const;
	const BlockFeature* getFeatureByType(const BlockFeatureType* blockFeatureType) const;
	BlockFeature* getFeatureByType(const BlockFeatureType* blockFeatureType);
	std::string describeFeatures() const;
	void removeFeature(const BlockFeatureType* blockFeatureType);

	std::string toS() const;
};

#include "../src/block.hpp"
class Actor final : public BaseActor<Block, Actor, Area>
{
public:
	std::unordered_set<Actor*> m_canSee;
	void (*m_onTaskComplete)(Actor& actor);

	Actor(Block* l, const Shape* s, const MoveType* mt);
	Actor(const Shape* s, const MoveType* mt);
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	bool isVisible(Actor* observer) const;

	void taskComplete();
	void doVision(std::unordered_set<Actor*>&& actors);
	void doFall(uint32_t distance, Block* block);
	void exposedToFluid(const FluidType* fluidType);
	bool canSee(const Actor& actor) const;
};
#include "../src/actor.hpp"
class Area final : public BaseArea<Block, Actor, Area>
{
public:
	Area(uint32_t x, uint32_t y, uint32_t z) : BaseArea<Block, Actor, Area>(x, y, z) {}
	void notifyNoRouteFound(Actor& actor);
};
#include "../src/area.hpp"

// Can anyone enter ever?
bool Block::anyoneCanEnterEver() const
{
	if(isSolid())
		return false;
	for(const BlockFeature& blockFeature : m_features)
	{
		if(blockFeature.blockFeatureType == s_fortification || blockFeature.blockFeatureType == s_floodGate ||
				(blockFeature.blockFeatureType == s_door && blockFeature.locked)
		  )
			return false;
	}
	return true;
}
// Can this moveType enter ever?
bool Block::moveTypeCanEnter(const MoveType* moveType) const
{
	// Swiming.
	for(auto [fluidType, pair] : m_fluids)
	{
		auto found = moveType->swim.find(fluidType);
		if(found != moveType->swim.end() && found->second <= pair.first)
			return true;
	}
	// Not swimming and fluid level is too high.
	if(m_totalFluidVolume > Config::maxBlockVolume / 2)
		return false;
	// Not flying and either not walking or ground is not supported.
	if(!moveType->fly && (!moveType->walk || !canStandIn()))
	{
		if(moveType->climb < 2)
			return false;
		else
		{
			// Only climb2 moveTypes can enter.
			for(Block* block : getAdjacentOnSameZLevelOnly())
				//TODO: check for climable features?
				if(block->isSupport())
					return true;
			return false;
		}
	}
	return true;
}
// Get a move cost for moving from a block onto this one for a given move type.
uint32_t Block::moveCost(const MoveType* moveType, Block* from) const
{
	if(moveType->fly)
		return 10;
	for(auto [fluidType, volume] : moveType->swim)
		if(volumeOfFluidTypeContains(fluidType) >= volume)
			return 10;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(m_z > from->m_z && !hasFeatureType(s_ramp))
		return 20;
	return 10;
}
// Checks if a move type can enter from a block. This check is seperate from checking if the shape and move type can enter at all.
bool Block::moveTypeCanEnterFrom(const MoveType* moveType, Block* from) const
{
	for(auto [fluidType, volume] : moveType->swim)
	{
		// Can travel within and enter liquid from any angle.
		if(volumeOfFluidTypeContains(fluidType) >= volume)
			return true;
		// Can leave liquid at any angle.
		if(from->volumeOfFluidTypeContains(fluidType) >= volume)
			return true;
	}
	// Can always enter on same z level.
	if(m_z == from->m_z)
		return true;
	// Cannot go up if:
	if(m_z > from->m_z)
		for(const BlockFeature& blockFeature : m_features)
			if(blockFeature.blockFeatureType == s_floor || blockFeature.blockFeatureType == s_floorGrate ||
					(blockFeature.blockFeatureType == s_hatch && blockFeature.locked)
			  )
				return false;
	// Cannot go down if:
	if(m_z < from->m_z)
		for(const BlockFeature& blockFeature : from->m_features)
			if(blockFeature.blockFeatureType == s_floor || blockFeature.blockFeatureType == s_floorGrate ||
					(blockFeature.blockFeatureType == s_hatch && blockFeature.locked)
			  )
				return false;
	// Can enter from any angle if flying or climbing.
	if(moveType->fly || moveType->climb > 0)
		return true;
	// Can go up if from contains a ramp or up stairs.
	if(m_z > from->m_z && (from->hasFeatureType(s_ramp) || from->hasFeatureType(s_upStairs) || from->hasFeatureType(s_upDownStairs)))
		return true;
	// Can go down if this contains a ramp or from contains down stairs and this contains up stairs.
	if(m_z < from->m_z && (hasFeatureType(s_ramp) || (
					(from->hasFeatureType(s_downStairs) || from->hasFeatureType(s_upDownStairs)) &&
					(hasFeatureType(s_upStairs) || hasFeatureType(s_upDownStairs))
					)))
		return true;
	return false;
}
bool Block::canStandIn() const
{
	assert(m_adjacents.at(0) != nullptr);
	if(m_adjacents[0]->isSolid())
		return true;
	if(hasFeatureType(s_floor))
		return true;
	if(hasFeatureType(s_upStairs))
		return true;
	if(hasFeatureType(s_downStairs))
		return true;
	if(hasFeatureType(s_upDownStairs))
		return true;
	if(hasFeatureType(s_ramp))
		return true;
	if(hasFeatureType(s_floorGrate))
		return true;
	if(hasFeatureType(s_hatch))
		return true;
	if(m_adjacents[0]->hasFeatureType(s_upStairs))
		return true;
	if(m_adjacents[0]->hasFeatureType(s_upDownStairs))
		return true;
	// Neccessary for multi tile actors to use ramps.
	if(m_adjacents[0]->hasFeatureType(s_ramp))
		return true;
	return false;
}
// Replace getAdjacentWithEdgeAndCornerAdjacent with either getAdjacentWithEdgeAdjacent or m_adajcentsVector depending on if you allow diagonal movement via edges or corners.
void Block::clearMoveCostsCacheForSelfAndAdjacent()
{
	// Clear move costs cache for adjacent and self.
	m_moveCostsCache.clear();
	for(Block* adjacent : getAdjacentWithEdgeAndCornerAdjacent())
		adjacent->m_moveCostsCache.clear();
}
bool Block::canSeeIntoFromAlways(const Block* block) const
{
	if(isSolid() && !getSolidMaterial()->transparent)
		return false;
	if(hasFeatureType(s_door))
		return false;
	// looking up.
	if(m_z > block->m_z)
	{
		const BlockFeature* floor = getFeatureByType(s_floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		if(hasFeatureType(s_hatch))
			return false;
	}
	// looking down.
	if(m_z < block->m_z)
	{
		const BlockFeature* floor = block->getFeatureByType(s_floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		if(block->hasFeatureType(s_hatch))
			return false;
	}
	return true;
}
bool Block::canSeeThrough() const
{
	if(isSolid() && !getSolidMaterial()->transparent)
		return false;
	const BlockFeature* door = getFeatureByType(s_door);
	if(door != nullptr && !door->materialType->transparent && door->closed)
		return false;
	return true;
}
bool Block::canSeeThroughFrom(const Block& block) const
{
	if(!canSeeThrough())
		return false;
	if(m_z == block.m_z)
		return true;
	// looking up.
	if(m_z > block.m_z)
	{
		const BlockFeature* floor = getFeatureByType(s_floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = getFeatureByType(s_hatch);
		if(hatch != nullptr && !hatch->materialType->transparent && hatch->closed)
			return false;
	}
	// looking down.
	if(m_z < block.m_z)
	{
		const BlockFeature* floor = block.getFeatureByType(s_floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = block.getFeatureByType(s_hatch);
		if(hatch != nullptr && !hatch->materialType->transparent && hatch->closed)
			return false;
	}
	return true;
}
float Block::visionDistanceModifier() const
{
	return 1;
}
bool Block::fluidCanEnterEver() const
{
	if(isSolid())
		return false;
	return true;
}
bool Block::fluidCanEnterEver(const FluidType* fluidType) const
{
	(void)fluidType;
	return true;
}
bool Block::isSupport() const
{
	return isSolid();
}
uint32_t Block::getMass() const
{
	if(!isSolid())
		return 0;
	else
		return getSolidMaterial()->mass;
}
std::vector<std::pair<Block*, uint32_t>> Block::getMoveCosts(const Shape* shape, const MoveType* moveType)
{
	std::vector<std::pair<Block*, uint32_t>> output;
	// Directly adjacent.
	for(Block* block : getAdjacentWithEdgeAndCornerAdjacent())
		if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType) && block->moveTypeCanEnterFrom(moveType, this))
			output.emplace_back(block, block->moveCost(moveType, static_cast<Block*>(this)));
	// Diagonal move down.
	if(moveType->climb > 0 || moveType->jumpDown)
	{
		for(Block* block : getEdgeAdjacentOnlyOnNextZLevelDown())
			if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType))
				output.emplace_back(block, block->moveCost(moveType, static_cast<Block*>(this)));
	}
	// Diagonal move down if entering fluid.
	else if(!moveType->swim.empty() && m_adjacents[0]->isSolid())
		for(Block* block : getEdgeAdjacentOnlyOnNextZLevelDown())
			if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType))
				for(auto [fluidType, volume] : moveType->swim)
					if(block->volumeOfFluidTypeContains(fluidType) >= volume)
						output.emplace_back(block, block->moveCost(moveType, static_cast<Block*>(this)));
	// Diagonal move up.
	if(moveType->climb > 0)
	{
		for(Block* block : getEdgeAdjacentOnlyOnNextZLevelUp())
			if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType))
				output.emplace_back(block, block->moveCost(moveType, static_cast<Block*>(this)));
	}
	// Diagonal move up if exiting fluid.
	else if(!moveType->swim.empty() && m_totalFluidVolume > Config::maxBlockVolume / 2)
		for(auto [fluidType, volume] : moveType->swim)
		{
			assert(fluidType != nullptr);
			if(volumeOfFluidTypeContains(fluidType) >= volume)
				for(Block* block : getEdgeAdjacentOnlyOnNextZLevelUp())
					if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType) && block->m_totalFluidVolume < Config::maxBlockVolume / 2 && block->canStandIn())
						output.emplace_back(block, block->moveCost(moveType, static_cast<Block*>(this)));
		}
	// Vertical move up.
	if(moveType->climb > 1)
		for(Block* block : m_adjacents[5]->getAdjacentOnSameZLevelOnly())
			if(block->isSolid() && m_adjacents[5]->anyoneCanEnterEver() && m_adjacents[5]->shapeAndMoveTypeCanEnterEver(shape, moveType))
				output.emplace_back(m_adjacents[5], block->moveCost(moveType, static_cast<Block*>(this)));
	return output;
}
void Block::moveContentsTo(Block* block)
{
	if(isSolid())
	{
		const MaterialType* materialType = getSolidMaterial();
		setNotSolid();
		block->setSolid(materialType);
	}
	//TODO: other stuff falls?
}
void Block::addConstructedFeature(const BlockFeatureType* blockFeatureType, const MaterialType* materialType)
{
	assert(!isSolid());
	m_features.emplace_back(blockFeatureType, materialType, false);
	clearMoveCostsCacheForSelfAndAdjacent();
	m_area->expireRouteCache();
	if(m_area->m_visionCuboidsActive && (blockFeatureType == s_floor || blockFeatureType == s_hatch) && !materialType->transparent)
		VisionCuboid<Block, Actor, Area>::BlockFloorIsSometimesOpaque(*this);
}
void Block::addHewnFeature(const BlockFeatureType* blockFeatureType, const MaterialType* materialType)
{
	if(materialType == nullptr)
	{
		assert(!isSolid());
		materialType = getSolidMaterial();
	}
	m_features.emplace_back(blockFeatureType, materialType, true);
	// clear move cost cache and expire route cache happen implicitly with call to setNotSolid.
	if(isSolid())
		setNotSolid();
	if(m_area->m_visionCuboidsActive && (blockFeatureType == s_floor || blockFeatureType == s_hatch) && !materialType->transparent)
		VisionCuboid<Block, Actor, Area>::BlockFloorIsSometimesOpaque(*this);
}
bool Block::hasFeatureType(const BlockFeatureType* blockFeatureType) const
{
	for(const BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType == blockFeatureType)
			return true;
	return false;
}
const BlockFeature* Block::getFeatureByType(const BlockFeatureType* blockFeatureType) const
{
	auto output = std::find_if(m_features.begin(), m_features.end(), [&](const BlockFeature& blockFeature){ return blockFeature.blockFeatureType == blockFeatureType; });
	if(output == m_features.end())
		return nullptr;
	return &*output;
}
BlockFeature* Block::getFeatureByType(const BlockFeatureType* blockFeatureType)
{
	return const_cast<BlockFeature*>(const_cast<const Block*>(this)->getFeatureByType(blockFeatureType));
}
void Block::removeFeature(const BlockFeatureType* blockFeatureType)
{
	auto found = std::find_if(m_features.begin(), m_features.end(), [&](BlockFeature& blockFeature){ return blockFeature.blockFeatureType == blockFeatureType; });
	if(found != m_features.end())
		m_features.erase(found);
	if(m_area->m_visionCuboidsActive && (blockFeatureType == s_floor || blockFeatureType == s_hatch) && !found->materialType->transparent)
		// block, opacity, floor
		VisionCuboid<Block, Actor, Area>::BlockFloorIsNeverOpaque(*this);
}
std::string Block::describeFeatures() const
{
	std::string output;
	for(const BlockFeature& blockFeature : m_features)
	{
		output +='#';
		output += blockFeature.materialType->name + " " +  blockFeature.blockFeatureType->name;
		if(blockFeature.closed)
			output += "closed";
		if(blockFeature.locked)
			output += "locked";
	}
	return output;
}
std::string Block::toS() const
{
	std::string materialName = isSolid() ? getSolidMaterial()->name: "empty";
	std::string output;
	output.reserve(materialName.size() + 16 + (m_fluids.size() * 12));
	output = std::to_string(m_x) + ", " + std::to_string(m_y) + ", " + std::to_string(m_z) + ": " + materialName + ";";
	for(auto& [fluidType, pair] : m_fluids)
		output += fluidType->name + ":" + std::to_string(pair.first) + ";";
	for(auto& pair : m_actors)
		output += ',' + pair.first->m_name;
	return output + describeFeatures();
}
Actor::Actor(Block* l, const Shape* s, const MoveType* mt) : BaseActor<Block, Actor, Area>(l, s, mt), m_onTaskComplete(nullptr) {}
Actor::Actor(const Shape* s, const MoveType* mt) : BaseActor(s, mt), m_onTaskComplete(nullptr) {}
uint32_t Actor::getSpeed() const
{
	return 2;
}
// Get current vision range.
uint32_t Actor::getVisionRange() const
{
	return 15;
}
// Check next task queue or go idle.
void Actor::taskComplete()
{
	if(m_onTaskComplete != nullptr)
		m_onTaskComplete(*this);
}
// Do fog of war and psycological effects.
void Actor::doVision(std::unordered_set<Actor*>&& actors)
{
	m_canSee = actors;
}
// Take fall damage, etc.
void Actor::doFall(uint32_t distance, Block* block)
{
	(void)distance;
	(void)block;
}
// Take temperature damage, get wet, get dirty, etc.
void Actor::exposedToFluid(const FluidType* fluidType)
{
	(void)fluidType;
}
bool Actor::canSee(const Actor& actor) const
{
	(void)actor;
	return true;
}
// Tell the player that an attempted pathing operation is not possible.
void Area::notifyNoRouteFound(Actor& actor) 
{ 
	(void)actor;
}

// typesRegistered is used to make sure registerTypes is called only once durring testing.
bool typesRegistered = false;
void registerTypes()
{
	if(typesRegistered)
		return;
	typesRegistered = true;

	// name, viscosity, density, mist duration, mist spread
	s_water = registerFluidType("water", 100, 100, 10, 2);
	s_CO2 = registerFluidType("CO2", 200, 10, 0, 0);
	s_lava = registerFluidType("lava", 20, 300, 5, 1);
	s_mercury = registerFluidType("mercury", 50, 200, 0, 0);

	// name, density, transparent
	s_stone = registerMaterialType("stone", 100, false);
	s_glass = registerMaterialType("glass", 100, true);

	// name, offsets and volumes
	s_oneByOneFull = registerShape("oneByOneFull", {{0,0,0,100}});
	s_oneByOneHalfFull = registerShape("oneByOneHalfFull", {{0,0,0,50}});
	s_oneByOneQuarterFull = registerShape("oneByOneQuarterFull", {{0,0,0,25}});
	s_twoByTwoFull = registerShape("twoByTwoFull", {{0,0,0,100}, {1,0,0,100}, {0,1,0,100}, {1,1,0,100}});

	// name, swim, climb, jumpDown, fly
	std::unordered_map<const FluidType*, uint32_t> noSwim;
	s_twoLegs = registerMoveType("two legs", true, noSwim, 0, false, false);
	s_fourLegs = registerMoveType("four legs", true, noSwim, 0, false, false);
	s_flying = registerMoveType("flying", false, noSwim, 0, false, true);
	std::unordered_map<const FluidType*, uint32_t> swimInWater = {{s_water, Config::maxBlockVolume / 2}};
	s_swimmingInWater = registerMoveType("swimming in water", false, swimInWater, 0, false, false);
	s_twoLegsAndSwimmingInWater = registerMoveType("two legs and swimming in water", true, swimInWater, 0, false, false);
	s_fourLegsAndSwimmingInWater = registerMoveType("four legs and swimming in water", true, swimInWater, 0, false, false);
	std::unordered_map<const FluidType*, uint32_t> swimInMercury = {{s_mercury, Config::maxBlockVolume / 2}};
	s_swimmingInMercury = registerMoveType("swimming in mercury", false, swimInMercury, 0, false, false);
	s_twoLegsAndSwimmingInMercury = registerMoveType("two legs and swimming in mercury", true, swimInMercury, 0, false, false);
	s_twoLegsAndClimb1 = registerMoveType("two legs and climb1", true, noSwim, 1, false, false);
	s_twoLegsAndClimb2 = registerMoveType("two legs and climb2", true, noSwim, 2, false, false);

	// name
	s_floor = registerBlockFeatureType("floor");
	s_upStairs = registerBlockFeatureType("up stairs");
	s_downStairs = registerBlockFeatureType("down stairs");
	s_upDownStairs = registerBlockFeatureType("up/down stairs");
	s_ramp = registerBlockFeatureType("ramp");
	s_fortification = registerBlockFeatureType("fortification");
	s_door = registerBlockFeatureType("door");
	s_hatch = registerBlockFeatureType("hatch");
	s_floorGrate = registerBlockFeatureType("floor grate");
	s_floodGate = registerBlockFeatureType("flood gate");
}

// Test helpers.
void setSolidLayer(Area& area, uint32_t z, const MaterialType* materialType)
{
	for(uint32_t x = 0; x != area.m_sizeX; ++x)
		for(uint32_t y = 0; y != area.m_sizeY; ++y)
			area.m_blocks[x][y][z].setSolid(materialType);
}
void setSolidLayers(Area& area, uint32_t zbegin, uint32_t zend, const MaterialType* materialType)
{
	for(;zbegin <= zend; ++zbegin)
		setSolidLayer(area, zbegin, materialType);
}
void setSolidWalls(Area& area, uint32_t height, const MaterialType* materialType)
{	
	for(uint32_t z = 0; z != height + 1; ++ z)
	{
		for(uint32_t x = 0; x != area.m_sizeX; ++x)
		{
			area.m_blocks[x][0][z].setSolid(materialType);
			area.m_blocks[x][area.m_sizeY - 1][z].setSolid(materialType);
		}
		for(uint32_t y = 0; y != area.m_sizeY; ++y)
		{
			area.m_blocks[0][y][z].setSolid(materialType);
			area.m_blocks[area.m_sizeX - 1][y][z].setSolid(materialType);
		}
	}
}
void setFullFluidCuboid(Block& low, Block& high, const FluidType* fluidType)
{
	assert(low.m_totalFluidVolume == 0);
	assert(low.fluidCanEnterEver());
	assert(high.m_totalFluidVolume == 0);
	assert(high.fluidCanEnterEver());
	BaseCuboid<Block, Actor, Area> cuboid(high, low);
	for(Block& block : cuboid)
	{
		assert(block.m_totalFluidVolume == 0);
		assert(block.fluidCanEnterEver());
		block.addFluid(100, fluidType);
	}
}
void validateAllBlockFluids(Area& area)
{
	for(uint32_t x = 0; x < area.m_sizeX; ++x)
		for(uint32_t y = 0; y < area.m_sizeY; ++y)
			for(uint32_t z = 0; z < area.m_sizeZ; ++z)
				for(auto [fluidType, pair] : area.m_blocks[x][y][z].m_fluids)
					assert(pair.second->m_fluidType == fluidType);
}
// Get one fluid group with the specified type. Return null if there is more then one.
FluidGroup<Block, Area>* getFluidGroup(Area& area, const FluidType* fluidType)
{
	FluidGroup<Block, Area>* output = nullptr;
	for(FluidGroup<Block, Area>& fluidGroup : area.m_fluidGroups)
		if(fluidGroup.m_fluidType == fluidType)
		{
			if(output != nullptr)
				return nullptr;
			output = &fluidGroup;
		}
	return output;
}
std::string toS(std::unordered_set<Block*>& blocks)
{
	std::string output = "set of " + std::to_string(blocks.size()) + " blocks";
	for(Block* block : blocks)
		output +='*' + block->toS();
	return output;
}
