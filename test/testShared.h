/*
 * Basic setup for lib_rock_mechanism. Users should fill out the 4 methods below and add custom member data to the three class declarations. 
 * Will be primarily interacted with via Area::step, Block::setSolid, Actor::setPosition, and Actor::setDestination.
 */
#include <cstdint>
const static uint32_t s_maxBlockVolume = 100;
// 1 for testing only, otherwise should be higher.
const static uint32_t s_actorDoVisionFrequency = 1;
const static uint32_t s_pathHuristicConstant = 1;
const static float s_maxDistanceVisionModifier = 1.1;
const static uint32_t s_locationBucketSize = 5;
const static bool s_fluidPiston = true;
// How many units seep through each step max = viscosity / seepDiagonalModifier.
// Disable by setting to 0.
const static uint32_t s_fluidsSeepDiagonalModifier = 100;

#include "../block.h"
#include "../actor.h"
#include "../locationBuckets.h"
#include "../area.h"
#include "../moveType.h"
#include "../fluidType.h"
#include "../materialType.h"
#include "../shape.h"
#include "../visionRequest.h"
#include "../routeRequest.h"
#include "../fluidGroup.h"
#include "../hasScheduledEvents.h"
#include "../mistDisperseEvent.h"
//#include "../room.h"

static uint32_t s_step;
BS::thread_pool_light s_pool;

const static MoveType* s_twoLegs;
const static MoveType* s_fourLegs;
const static MoveType* s_flying;
const static FluidType* s_water;
const static FluidType* s_CO2;
const static FluidType* s_lava;
const static FluidType* s_mercury;
const static MaterialType* s_stone;
const static Shape* s_oneByOneFull;
const static Shape* s_oneByOneHalfFull;
const static Shape* s_oneByOneQuarterFull;
const static Shape* s_twoByTwoFull;


// Put custom member data declarations here
class Block : public baseBlock
{
public:
	bool anyoneCanEnterEver() const;
	bool moveTypeCanEnter(const MoveType* moveType) const;
	bool canEnterCurrently(Actor* actor) const;
	uint32_t moveCost(const MoveType* moveType, Block* origin) const;
	bool canStandOn() const;

	bool canSeeThroughFrom(Block* block) const;
	float visionDistanceModifier() const;

	bool fluidCanEnterEver() const;
	bool fluidCanEnterEver(const FluidType* fluidType) const;

	bool isSupport() const;
	uint32_t getMass() const;
};

class Actor : public baseActor
{
public:
	Actor(Block* l, const Shape* s, const MoveType* mt);
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	bool isVisible(Actor* observer) const;

	void taskComplete();
	void doVision(std::unordered_set<Actor*> actors);
	void doFall(uint32_t distance, Block* block);
	void exposedToFluid(const FluidType* fluidType);
	bool canSee(Actor* actor) const;
};

class Area : public baseArea
{
public:
	Area(uint32_t x, uint32_t y, uint32_t z) : baseArea(x, y, z) {}
	void notifyNoRouteFound(Actor* actor);
};

#include "../block.cpp"
#include "../actor.cpp"
#include "../visionRequest.cpp"
#include "../routeRequest.cpp"
#include "../fluidQueue.cpp"
#include "../fillQueue.cpp"
#include "../drainQueue.cpp"
#include "../fluidGroup.cpp"
#include "../area.cpp"
#include "../caveIn.cpp"
#include "../hasScheduledEvents.cpp"
//#include "../room.cpp"
#include "../locationBuckets.cpp"
#include "../mistDisperseEvent.cpp"

// Can anyone enter ever?
bool Block::anyoneCanEnterEver() const
{
	return !isSolid();
}
// Can this moveType enter ever?
bool Block::moveTypeCanEnter(const MoveType* moveType) const
{
	if(moveType != s_flying && !m_adjacents[0]->canStandOn())
		return false;
	return true;
}
// Is this actor prevented from entering based on something other then move type or size?
bool Block::canEnterCurrently(Actor* actor) const
{
	return true;
}
// Get a move cost for moving from a block onto this one for a given move type.
uint32_t Block::moveCost(const MoveType* moveType, Block* from) const
{
	return 10;
}
bool Block::canStandOn() const
{
	return isSolid();
}
bool Block::canSeeThroughFrom(Block* block) const
{
	return !isSolid();
}
float Block::visionDistanceModifier() const
{
	return 1;
}
bool Block::fluidCanEnterEver() const
{
	return !isSolid();
}
bool Block::fluidCanEnterEver(const FluidType* fluidType) const
{
	return !isSolid();
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

Actor::Actor(Block* l, const Shape* s, const MoveType* mt) : baseActor(l, s, mt) {}
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
}
// Do fog of war and psycological effects.
void Actor::doVision(std::unordered_set<Actor*> actors)
{
}
// Take fall damage, etc.
void Actor::doFall(uint32_t distance, Block* block)
{
}
// Take temperature damage, get wet, get dirty, etc.
void Actor::exposedToFluid(const FluidType* fluidType)
{
}
bool Actor::canSee(Actor* actor) const
{
	return true;
}
// Tell the player that an attempted pathing operation is not possible.
void Area::notifyNoRouteFound(Actor* actor) { }

void registerTypes()
{
	s_twoLegs = registerMoveType("two legs");
	s_fourLegs = registerMoveType("four legs");
	s_flying = registerMoveType("flying");

	// name, viscosity, density, mist duration, mist spread
	s_water = registerFluidType("water", 100, 100, 10, 2);
	s_CO2 = registerFluidType("CO2", 200, 10, 0, 0);
	s_lava = registerFluidType("lava", 20, 200, 5, 1);
	s_mercury = registerFluidType("mercury", 50, 200, 0, 0);

	// name, density
	s_stone = registerMaterialType("stone", 100);

	// name, offsets and volumes
	s_oneByOneFull = registerShape("oneByOneFull", {{0,0,0,100}});
	s_oneByOneHalfFull = registerShape("oneByOneHalfFull", {{0,0,0,50}});
	s_oneByOneQuarterFull = registerShape("oneByOneQuarterFull", {{0,0,0,25}});
	s_twoByTwoFull = registerShape("twoByTwoFull", {{0,0,0,100}, {1,0,0,100}, {0,1,0,100}, {1,1,0,100}});
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
std::string toS(std::unordered_set<Block*>& blocks)
{
	std::string output = "set of " + std::to_string(blocks.size()) + " blocks";
	for(Block* block : blocks)
		output +='*' + block->toS();
	return output;
}
