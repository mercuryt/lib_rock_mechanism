/*
 * Basic setup for lib_rock_mechanism. Users should fill out the 4 methods below and add custom member data to the three class declarations. 
 * Will be primarily interacted with via Area::step, Block::setSolid, Actor::setPosition, and Actor::setDestination.
 */

#define MAX_BLOCK_VOLUME 100
#define CACHEABLE_VISION_RANGE 15
// 1 for testing only, otherwise should be higher.
#define ACTOR_DO_VISION_FREQUENCY 1
#define PATH_HURISTIC_CONSTANT 1

#include "../block.h"
#include "../actor.h"
#include "../area.h"
#include "../moveType.h"
#include "../fluidType.h"
#include "../materialType.h"
#include "../shape.h"
#include "../visionRequest.h"
#include "../routeRequest.h"
#include "../fluidGroup.h"
#include "../hasScheduledEvents.h"

static uint32_t s_step = 0;
BS::thread_pool_light s_pool;

// Put custom member data declarations here
class Block : public baseBlock
{
public:
	bool canEnterEver() const;
	bool moveTypeCanEnter(MoveType* moveType) const;
	bool canEnterCurrently(Actor* actor) const;
	uint32_t getMoveCost(MoveType* moveType, Block* origin) const;

	bool canSeeThrough() const;

	bool fluidCanEnterEver() const;
	bool fluidCanEnterEver(FluidType* fluidType) const;

	bool isSupport() const;
	uint32_t getMass() const;

	void moveContentsTo(Block* block);
};

class Actor : public baseActor
{
public:
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	bool isVisible(Actor* observer) const;

	void taskComplete();
	void doVision(std::unordered_set<Actor*> actors);
	void doFall(uint32_t distance, Block* block);
	void exposedToFluid(FluidType* fluidType);
};

class Area : public baseArea
{
public:
	void notifyNoRouteFound(Actor* actor);
};

#include "../block.cpp"
#include "../actor.cpp"
#include "../visionRequest.cpp"
#include "../routeRequest.cpp"
#include "../fluidGroup.cpp"
#include "../area.cpp"
#include "../caveIn.cpp"
#include "../hasScheduledEvents.cpp"

// Can anyone enter ever?
bool Block::canEnterEver() const
{
	return m_solid == nullptr;
}
// Can this moveType enter ever?
bool Block::moveTypeCanEnter(MoveType* moveType) const
{
	return true;
}
// Get a move cost for moving from a block onto this one for a given move type.
uint32_t Block::getMoveCost(MoveType* moveType, Block* from) const
{
	return 10;
}
bool Block::canSeeThrough() const
{
	return m_solid == nullptr;
}
bool Block::fluidCanEnterEver() const
{
	return m_solid == nullptr;
}
bool Block::fluidCanEnterEver(FluidType* fluidType) const
{
	return m_solid == nullptr;
}
bool Block::isSupport() const
{
	return m_solid != nullptr;
}
uint32_t Block::getMass() const
{
	if(m_solid == nullptr)
		return 0;
	else
		return m_solid->mass;
}


uint32_t Actor::getSpeed() const
{
	return 2;
}
// Get current vision range.
uint32_t Actor::getVisionRange() const
{
	return CACHEABLE_VISION_RANGE;
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
// Take temperateru damage, get wet, get dirty, etc.
void Actor::exposedToFluid(FluidType* fluidType)
{
}
// Tell the player that an attempted pathing operation is not possible.
void Area::notifyNoRouteFound(Actor* actor) { }

void registerTypes()
{
	MoveType* twoLegs = registerMoveType("two legs");
	MoveType* fourLegs = registerMoveType("four legs");

	// name, viscosity, density
	FluidType* water = registerFluidType("water", 100, 100);
	FluidType* CO2 = registerFluidType("CO2", 100, 10);
	FluidType* lava = registerFluidType("lava", 100, 200);

	// name, density
	MaterialType* stone = registerMaterialType("stone", 100);

	// name, offsets and volumes
	Shape* oneByOneFull = registerShape("oneByOneFull", {{0,0,0,100}});
	Shape* oneByOneHalfFull = registerShape("oneByOneHalfFull", {{0,0,0,50}});
	Shape* oneByOneQuarterFull = registerShape("oneByOneQuarterFull", {{0,0,0,25}});
	Shape* twoByTwoFull = registerShape("twoByTwoFull", {{0,0,0,100}, {1,0,0,100}, {0,1,0,100}, {1,1,0,100}});
}
