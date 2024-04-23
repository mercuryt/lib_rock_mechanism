/*
 * Example config used for testing. Attempts to replicate Dwarf Fortress mechanics regarding vision, pathing, cave in, and fluids; except max fluid volume is 100 rather then 7.
 */
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <list>
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
	constexpr uint32_t daysPerYear = 365;
	constexpr uint32_t hoursPerDay = 24;
	constexpr uint32_t stepsPerHour = 5000;
	constexpr uint32_t stepsPerDay = stepsPerHour * hoursPerDay;
	constexpr uint32_t stepsPerYear = stepsPerDay * daysPerYear;
}

inline uint32_t s_step;
BS::thread_pool_light s_pool;

#include "../src/block.h"
#include "../src/actor.h"
#include "../src/area.h"

#include "../src/moveType.h"
#include "../src/materialType.h"
#include "../src/fluidType.h"
#include "../src/plant.h"
#include "../src/animal.h"
	
class FluidType : public BaseFluidType
{
public:
	FluidType(std::string name, uint32_t viscosity, uint32_t density, uint32_t mistDuration, uint32_t maxMistSpread) : BaseFluidType(name, viscosity, density, mistDuration, maxMistSpread) {}
	bool operator==(const FluidType& x) const { return this == &x; }
	bool operator==(const FluidType* x) const { return this == x; }
};
const static FluidType s_water("water", 100, 100, 10, 2);
const static FluidType s_CO2("CO2", 200, 10, 0, 0);
const static FluidType s_lava("lava", 20, 300, 5, 1);
const static FluidType s_mercury("mercury", 50, 200, 0, 0);

class MoveType : public BaseMoveType<FluidType> 
{
public:
	MoveType(std::string name, bool walk, std::unordered_map<const FluidType*, uint32_t> swim, uint32_t climb, bool jumpDown, bool fly) : BaseMoveType<FluidType>(name, walk, swim, climb, jumpDown, fly) {}
	bool operator==(const MoveType& x) const { return this == &x; }
};
const static std::unordered_map<const FluidType*, uint32_t> noSwim;
const static MoveType s_twoLegs("two legs", true, noSwim, 0, false, false);
const static MoveType s_fourLegs("four legs", true, noSwim, 0, false, false);
const static MoveType s_flying("flying", false, noSwim, 0, false, true);
static std::unordered_map<const FluidType*, uint32_t> swimInWater = {{&s_water, Config::maxBlockVolume / 2}};
const static MoveType s_swimmingInWater("swimming in water", false, swimInWater, 0, false, false);
const static MoveType s_twoLegsAndSwimmingInWater("two legs and swimming in water", true, swimInWater, 0, false, false);
const static MoveType s_fourLegsAndSwimmingInWater("four legs and swimming in water", true, swimInWater, 0, false, false);
static std::unordered_map<const FluidType*, uint32_t> swimInMercury = {{&s_mercury, Config::maxBlockVolume / 2}};
const static MoveType s_swimmingInMercury("swimming in mercury", false, swimInMercury, 0, false, false);
const static MoveType s_twoLegsAndSwimmingInMercury("two legs and swimming in mercury", true, swimInMercury, 0, false, false);
const static MoveType s_twoLegsAndClimb1("two legs and climb1", true, noSwim, 1, false, false);
const static MoveType s_twoLegsAndClimb2("two legs and climb2", true, noSwim, 2, false, false);

class MaterialType : public BaseMaterialType
{
public:
	MaterialType(std::string name, uint32_t mass, bool transparent, uint32_t ignitionTemperature, uint32_t flameTemperature, uint32_t burnStageDuration, uint32_t flameStageDuration) : BaseMaterialType(name, mass, transparent, ignitionTemperature, flameTemperature, burnStageDuration, flameStageDuration) {}
	bool operator==(const MaterialType& x) const { return this == &x; }
};
// name, swim, climb, jumpDown, fly
const static MaterialType s_stone("stone", 100, false, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX);
const static MaterialType s_glass("glass", 100, true, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX);
const static MaterialType s_wood("wood", 10, false, 533, 1172, 10000, 40000);
const static MaterialType s_dirt("dirt", 50, false, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX);
// name, offsets and volumes
const static Shape s_oneByOneFull("oneByOneFull", {{0,0,0,100}});
const static Shape s_oneByOneHalfFull("oneByOneHalfFull", {{0,0,0,50}});
const static Shape s_oneByOneQuarterFull("oneByOneQuarterFull", {{0,0,0,25}});
const static Shape s_twoByTwoFull("twoByTwoFull", {{0,0,0,100}, {1,0,0,100}, {0,1,0,100}, {1,1,0,100}});

class AnimalType : public BaseAnimalType<FluidType>
{
public:
	AnimalType(
			const std::string name,
			const uint32_t minimumTemperature,
			const uint32_t maximumTemperature,
			const uint32_t stepsTillDieTemperature,
			const FluidType& fluidType,
			const uint32_t stepsNeedsFluidFrequency,
			const uint32_t stepsTillDieWithoutFluid,
			const bool carnivore,
			const bool herbavore,
			const uint32_t stepsNeedsFoodFrequency,
			const uint32_t stepsTillDieWithoutFood,
			const uint32_t stepsTillFullyGrown,
			const uint32_t visionRange,
			const uint32_t moveSpeed,
			const uint32_t mass
		  ) : BaseAnimalType<FluidType>(name, minimumTemperature, maximumTemperature, stepsTillDieTemperature, fluidType, stepsNeedsFluidFrequency, stepsTillDieWithoutFluid, carnivore, herbavore, stepsNeedsFoodFrequency, stepsTillDieWithoutFood, stepsTillFullyGrown, visionRange, moveSpeed, mass) {}
	bool canEat(const Plant& plant) const;
	bool canEat(const Actor& actor) const;
	bool willHunt(const Actor& actor) const;
	void onDeath();
};
// name, min temp, max temp, steps till die from temp, fluid type, needs fluid frequency, steps til die of thirst, eats meat, eats plants, hunger frequencey, steps till starve, steps of growth till adult size, vision distance, move speed, mass
const static AnimalType s_rabbit("rabbit", 261, 316, 5 * Config::stepsPerDay, s_water, 1 * Config::stepsPerDay, 3 * Config::stepsPerDay, false, true, 2 * Config::stepsPerDay, 10 * Config::stepsPerDay, 150 * Config::stepsPerDay, 10, 3, 2);
const static AnimalType s_fox("fox", 261, 316, 5 * Config::stepsPerDay, s_water, 1 * Config::stepsPerDay, 3 * Config::stepsPerDay, true, true, 2 * Config::stepsPerDay, 10 * Config::stepsPerDay, Config::stepsPerYear, 15, 4, 5);
const static AnimalType s_bear("bear", 220, 310, 10 * Config::stepsPerDay, s_water, 1 * Config::stepsPerDay, 10 * Config::stepsPerDay, true, true, 4 * Config::stepsPerDay, 15 * Config::stepsPerDay , 10 * Config::stepsPerYear, 10, 4, 200);
const static AnimalType s_deer("deer", 261, 310, 5 * Config::stepsPerDay, s_water, 1 * Config::stepsPerDay, 3 * Config::stepsPerDay, false, true, 2 * Config::stepsPerDay, 10 * Config::stepsPerDay, 3 * Config::stepsPerYear, 10, 3, 80);

struct BlockFeatureType
{
	const std::string name;
	bool operator==(const BlockFeatureType& x) const { return this == &x; }
};
struct BlockFeature 
{
	// Use pointers here rather then references so BlockFeature is copy constructable and can be stored in a mutable vector.
	const BlockFeatureType* blockFeatureType;
	const MaterialType* materialType;
	bool hewn;
	bool closed;
	bool locked;
	BlockFeature(const BlockFeatureType& bft, const MaterialType& mt, bool h) : blockFeatureType(&bft), materialType(&mt), hewn(h), closed(true), locked(false) {}
};
// name
const static BlockFeatureType s_floor("floor");
const static BlockFeatureType s_upStairs("up stairs");
const static BlockFeatureType s_downStairs("down stairs");
const static BlockFeatureType s_upDownStairs("up/down stairs");
const static BlockFeatureType s_ramp("ramp");
const static BlockFeatureType s_fortification("fortification");
const static BlockFeatureType s_door("door");
const static BlockFeatureType s_hatch("hatch");
const static BlockFeatureType s_floorGrate("floor grate");
const static BlockFeatureType s_floodGate("flood gate");

class PlantType : public BasePlantType<FluidType>
{
public:
	std::wstring drawString;
	PlantType(
			// Base type members.
			const std::string n,
			const bool a,
			const uint32_t maxgt,
			const uint32_t mingt,
			const uint32_t stdft,
			const uint32_t snff,
			const uint32_t stdwf,
			const uint32_t stfg,
			const bool gisl,
			const uint32_t doyfg,
			const uint32_t steof,
			const uint32_t rrmax,
			const uint32_t rrmin,
			const FluidType& ft, 
			// Derived type members.
			const std::wstring ds
	) : BasePlantType<FluidType>(n, a, maxgt, mingt, stdft, snff, stdwf, stfg, gisl, doyfg, steof, rrmax, rrmin, ft),
		drawString(ds) {}
	bool operator==(const PlantType& x) const { return this == &x; }
};
// name, annual, maximum temperature, minimum temperature, steps till die from temperature, steps needs fluid frequency, steps till die without fluid, steps till growth event, grows in sunlight, day of year for harvest, steps till end of harvest, root range max, root range min, fluidType
const static PlantType s_grass("grass", false, 316, 283, Config::stepsPerDay, Config::stepsPerDay * 4, Config::stepsPerDay * 8, Config::stepsPerDay * 100, true, 200, 50, 2, 1, s_water, L"\"");

enum class TemperatureZone { Surface, Underground, LavaSea};

class Actor;
class Block;
class Area;
class Plant;

// Put custom member data declarations here
class Block final : public BaseBlock<Block, Actor, Area, FluidType, MoveType, MaterialType>
{
public:
	std::vector<BlockFeature> m_features;
	std::list<Plant*> m_plants;
	TemperatureZone m_temperatureZone;

	Block();
	bool anyoneCanEnterEver() const;
	bool moveTypeCanEnter(const MoveType& moveType) const;
	bool moveTypeCanEnterFrom(const MoveType& moveType, Block& from) const;
	uint32_t moveCost(const MoveType& moveType, Block& origin) const;
	std::vector<std::pair<Block*, uint32_t>> getMoveCosts(const Shape& shape, const MoveType& moveType);
	bool canStandIn() const;
	void clearMoveCostsCacheForSelfAndAdjacent();

	bool canSeeIntoFromAlways(const Block& block) const;
	bool canSeeThrough() const;
	bool canSeeThroughFrom(const Block& block) const;
	float visionDistanceModifier() const;

	bool fluidCanEnterEver() const;
	bool fluidCanEnterEver(const FluidType& fluidType) const;

	bool isSupport() const;
	uint32_t getMass() const;

	void moveContentsTo(Block& block);

	void addConstructedFeature(const BlockFeatureType& blockFeatureType, const MaterialType& materialType);
	void addHewnFeature(const BlockFeatureType& blockFeatureType, const MaterialType* materialType = nullptr);
	bool hasFeatureType(const BlockFeatureType& blockFeatureType) const;
	BlockFeature* getFeatureByType(const BlockFeatureType& blockFeatureType);
	const BlockFeature* getFeatureByType(const BlockFeatureType& blockFeatureType) const;
	std::string describeFeatures() const;
	std::string describePlants() const;
	void removeFeature(const BlockFeatureType& blockFeatureType);

	uint32_t getAmbientTemperature() const;
	void applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature);

	void setExposedToSky(bool exposed);
	bool plantTypeCanGrow(const PlantType& plantType) const;

	std::string toS() const;
};
#include "../src/block.hpp"
class Actor final : public BaseActor<Actor, Block, MoveType, FluidType>
{
public:
	std::unordered_set<Actor*> m_canSee;
	void (*m_onTaskComplete)(Actor& actor);

	Actor(Block& l, const Shape& s, const MoveType& mt);
	Actor(const Shape& s, const MoveType& mt);
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	bool isVisible(Actor& observer) const;

	void taskComplete();
	void doVision(std::unordered_set<Actor*>&& actors);
	void doFall(uint32_t distance, Block& block);
	void exposedToFluid(const FluidType& fluidType);
	bool canSee(const Actor& actor) const;
};
class Area final : public BaseArea<Block, Actor, Area, FluidType>
{
public:
	const FluidType* m_currentlyRainingFluidType;
	uint32_t m_ambiantSurfaceTemperature;
	std::list<Plant> m_plants;
	ScheduledEvent* m_rainStopEvent;

	Area(uint32_t x, uint32_t y, uint32_t z) : BaseArea<Block, Actor, Area, FluidType>(x, y, z), m_currentlyRainingFluidType(nullptr), m_ambiantSurfaceTemperature(0) {}
	void notifyNoRouteFound(Actor& actor);
	void rain(const FluidType& fluidType, uint32_t duration);
	void setHour(uint32_t hour, uint32_t dayOfYear);
	void setDayOfYear(uint32_t dayOfYear);
	void setAmbientTemperatureFor(uint32_t hour, uint32_t dayOfYear);
	void setAmbientTemperature(uint32_t temperature);
};
#include "../src/area.hpp"
class Plant final : public BasePlant<Plant, PlantType, Block>
{
public:
	Plant(Block& l, const PlantType& pt, uint32_t pg = 0) : BasePlant<Plant, PlantType, Block>(l, pt, pg) {}
	void onDie();
	void onEndOfHarvest();
};
class RainStopEvent final : public ScheduledEvent
{
public:
	Area& m_area;
	RainStopEvent(uint32_t step, Area& a) : ScheduledEvent(step), m_area(a) {}
	~RainStopEvent() { m_area.m_rainStopEvent = nullptr; }
	void execute() { m_area.m_currentlyRainingFluidType = nullptr; }
};

Block::Block() : BaseBlock<Block, Actor, Area, FluidType, MoveType, MaterialType>(), m_temperatureZone(TemperatureZone::Surface) {}

// Can anyone enter ever?
// TODO: animals can't open doors.
bool Block::anyoneCanEnterEver() const
{
	if(isSolid())
		return false;
	for(const BlockFeature& blockFeature : m_features)
	{
		if(blockFeature.blockFeatureType == &s_fortification || blockFeature.blockFeatureType == &s_floodGate ||
				(blockFeature.blockFeatureType == &s_door && blockFeature.locked)
		  )
			return false;
	}
	return true;
}
// Can this moveType enter ever?
bool Block::moveTypeCanEnter(const MoveType& moveType) const
{
	// Swiming.
	for(auto& [fluidType, pair] : m_fluids)
	{
		auto found = moveType.swim.find(fluidType);
		if(found != moveType.swim.end() && found->second <= pair.first)
			return true;
	}
	// Not swimming and fluid level is too high.
	if(m_totalFluidVolume > Config::maxBlockVolume / 2)
		return false;
	// Not flying and either not walking or ground is not supported.
	if(!moveType.fly && (!moveType.walk || !canStandIn()))
	{
		if(moveType.climb < 2)
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
uint32_t Block::moveCost(const MoveType& moveType, Block& from) const
{
	if(moveType.fly)
		return 10;
	for(auto& [fluidType, volume] : moveType.swim)
		if(volumeOfFluidTypeContains(*fluidType) >= volume)
			return 10;
	// Double cost to go up if not fly, swim, or ramp (if climb).
	if(m_z > from.m_z && !hasFeatureType(s_ramp))
		return 20;
	return 10;
}
// Checks if a move type can enter from a block. This check is seperate from checking if the shape and move type can enter at all.
bool Block::moveTypeCanEnterFrom(const MoveType& moveType, Block& from) const
{
	for(auto& [fluidType, volume] : moveType.swim)
	{
		// Can travel within and enter liquid from any angle.
		if(volumeOfFluidTypeContains(*fluidType) >= volume)
			return true;
		// Can leave liquid at any angle.
		if(from.volumeOfFluidTypeContains(*fluidType) >= volume)
			return true;
	}
	// Can always enter on same z level.
	if(m_z == from.m_z)
		return true;
	// Cannot go up if:
	if(m_z > from.m_z)
		for(const BlockFeature& blockFeature : m_features)
			if(blockFeature.blockFeatureType == &s_floor || blockFeature.blockFeatureType == &s_floorGrate ||
					(blockFeature.blockFeatureType == &s_hatch && blockFeature.locked)
			  )
				return false;
	// Cannot go down if:
	if(m_z < from.m_z)
		for(const BlockFeature& blockFeature : from.m_features)
			if(blockFeature.blockFeatureType == &s_floor || blockFeature.blockFeatureType == &s_floorGrate ||
					(blockFeature.blockFeatureType == &s_hatch && blockFeature.locked)
			  )
				return false;
	// Can enter from any angle if flying or climbing.
	if(moveType.fly || moveType.climb > 0)
		return true;
	// Can go up if from contains a ramp or up stairs.
	if(m_z > from.m_z && (from.hasFeatureType(s_ramp) || from.hasFeatureType(s_upStairs) || from.hasFeatureType(s_upDownStairs)))
		return true;
	// Can go down if this contains a ramp or from contains down stairs and this contains up stairs.
	if(m_z < from.m_z && (hasFeatureType(s_ramp) || (
					(from.hasFeatureType(s_downStairs) || from.hasFeatureType(s_upDownStairs)) &&
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
bool Block::canSeeIntoFromAlways(const Block& block) const
{
	if(isSolid() && !getSolidMaterial().transparent)
		return false;
	if(hasFeatureType(s_door))
		return false;
	// looking up.
	if(m_z > block.m_z)
	{
		const BlockFeature* floor = getFeatureByType(s_floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		if(hasFeatureType(s_hatch))
			return false;
	}
	// looking down.
	if(m_z < block.m_z)
	{
		const BlockFeature* floor = block.getFeatureByType(s_floor);
		if(floor != nullptr && !floor->materialType->transparent)
			return false;
		const BlockFeature* hatch = block.getFeatureByType(s_hatch);
		if(hatch != nullptr && !hatch->materialType->transparent)
			return false;
	}
	return true;
}
bool Block::canSeeThrough() const
{
	if(isSolid() && !getSolidMaterial().transparent)
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
bool Block::fluidCanEnterEver(const FluidType& fluidType) const
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
		return getSolidMaterial().mass;
}
std::vector<std::pair<Block*, uint32_t>> Block::getMoveCosts(const Shape& shape, const MoveType& moveType)
{
	std::vector<std::pair<Block*, uint32_t>> output;
	for(Block* block : getAdjacentWithEdgeAndCornerAdjacent())
		if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType) && block->moveTypeCanEnterFrom(moveType, *this))
			output.emplace_back(block, block->moveCost(moveType, *this));
	// Diagonal move down.
	if(moveType.climb > 0 || moveType.jumpDown)
	{
		for(Block* block : getEdgeAdjacentOnlyOnNextZLevelDown())
			if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType))
				output.emplace_back(block, block->moveCost(moveType, *this));
	}
	// Diagonal move down if entering fluid.
	else if(!moveType.swim.empty() && m_adjacents[0]->isSolid())
		for(Block* block : getEdgeAdjacentOnlyOnNextZLevelDown())
			if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType))
				for(auto& [fluidType, volume] : moveType.swim)
					if(block->volumeOfFluidTypeContains(*fluidType) >= volume)
						output.emplace_back(block, block->moveCost(moveType, *this));
	// Diagonal move up.
	if(moveType.climb > 0)
	{
		for(Block* block : getEdgeAdjacentOnlyOnNextZLevelUp())
			if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType))
				output.emplace_back(block, block->moveCost(moveType, *this));
	}
	// Diagonal move up if exiting fluid.
	else if(!moveType.swim.empty() && m_totalFluidVolume > Config::maxBlockVolume / 2)
		for(auto& [fluidType, volume] : moveType.swim)
		{
			assert(fluidType != nullptr);
			if(volumeOfFluidTypeContains(*fluidType) >= volume)
				for(Block* block : getEdgeAdjacentOnlyOnNextZLevelUp())
					if(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(shape, moveType) && block->m_totalFluidVolume < Config::maxBlockVolume / 2 && block->canStandIn())
						output.emplace_back(block, block->moveCost(moveType, *this));
		}
	// Vertical move up.
	if(moveType.climb > 1)
		for(Block* block : m_adjacents[5]->getAdjacentOnSameZLevelOnly())
			if(block->isSolid() && m_adjacents[5]->anyoneCanEnterEver() && m_adjacents[5]->shapeAndMoveTypeCanEnterEver(shape, moveType))
				output.emplace_back(m_adjacents[5], block->moveCost(moveType, *this));
	return output;
}
void Block::moveContentsTo(Block& block)
{
	if(isSolid())
	{
		const MaterialType& materialType = getSolidMaterial();
		setNotSolid();
		block.setSolid(materialType);
	}
	//TODO: other stuff falls?
}
void Block::addConstructedFeature(const BlockFeatureType& blockFeatureType, const MaterialType& materialType)
{
	assert(!isSolid());
	m_features.emplace_back(blockFeatureType, materialType, false);
	clearMoveCostsCacheForSelfAndAdjacent();
	m_area->expireRouteCache();
	if((blockFeatureType == s_floor || blockFeatureType == s_hatch) && !materialType.transparent)
	{
		if(m_area->m_visionCuboidsActive)
			VisionCuboid<Block, Area>::BlockFloorIsSometimesOpaque(*this);
		setBelowNotExposedToSky();
	}
}
void Block::addHewnFeature(const BlockFeatureType& blockFeatureType, const MaterialType* materialType)
{
	if(materialType == nullptr)
	{
		assert(!isSolid());
		materialType = &getSolidMaterial();
	}
	m_features.emplace_back(blockFeatureType, *materialType, true);
	// clear move cost cache and expire route cache happen implicitly with call to setNotSolid.
	if(isSolid())
		setNotSolid();
	setBelowNotExposedToSky();
	if((blockFeatureType == s_floor || blockFeatureType == s_hatch) && !materialType->transparent)
	{
		if(m_area->m_visionCuboidsActive)
			VisionCuboid<Block, Area>::BlockFloorIsSometimesOpaque(*this);
		setBelowExposedToSky();
	}
}
bool Block::hasFeatureType(const BlockFeatureType& blockFeatureType) const
{
	for(const BlockFeature& blockFeature : m_features)
		if(blockFeature.blockFeatureType == &blockFeatureType)
			return true;
	return false;
}
// Can return nullptr.
BlockFeature* Block::getFeatureByType(const BlockFeatureType& blockFeatureType)
{
	auto output = std::find_if(m_features.begin(), m_features.end(), [&](const BlockFeature& blockFeature){ return blockFeature.blockFeatureType == &blockFeatureType; });
	if(output == m_features.end())
		return nullptr;
	return &*output;
}
const BlockFeature* Block::getFeatureByType(const BlockFeatureType& blockFeatureType) const
{
	return const_cast<const BlockFeature*>(const_cast<Block*>(this)->getFeatureByType(blockFeatureType));
}
void Block::removeFeature(const BlockFeatureType& blockFeatureType)
{
	BlockFeature* feature = getFeatureByType(blockFeatureType);
	if(feature == nullptr)
		return;
	bool transparentFloorOrHatch = m_area->m_visionCuboidsActive && (blockFeatureType == s_floor || blockFeatureType == s_hatch) && !feature->materialType->transparent;
	std::erase_if(m_features, [&](BlockFeature& blockFeature){ return blockFeature.blockFeatureType == &blockFeatureType; });
	if(transparentFloorOrHatch)
		VisionCuboid<Block, Area>::BlockFloorIsNeverOpaque(*this);
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
void Block::applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature)
{
	(void)oldTemperature;
	if(!m_fire)
	{
		if(isSolid())
		{
			const MaterialType& materialType = getSolidMaterial();
			if(materialType.ignitionTemperature <= newTemperature)
				m_fire = std::make_unique<Fire<Block, MaterialType>>(*this, materialType);
		}
		else if(!m_features.empty())
		{
			for(BlockFeature& blockFeature : m_features)
				if(blockFeature.materialType->ignitionTemperature <= newTemperature)
					m_fire = std::make_unique<Fire<Block, MaterialType>>(*this, *blockFeature.materialType);
		}
	}
}
void Block::setExposedToSky(bool exposed)
{
	m_exposedToSky = exposed;
	for(Plant* plant : m_plants)
		plant->updateGrowingStatus();
}
bool Block::plantTypeCanGrow(const PlantType& plantType) const
{
	return m_exposedToSky == plantType.growsInSunLight && canStandIn() && m_adjacents[0]->getSolidMaterial() == s_dirt;
}
std::string Block::describePlants() const
{
	std::string output;
	for(Plant* plant : m_plants)
		output += plant->m_plantType.name;
	return output;
}
//TODO: Use different inside temperatures?
uint32_t Block::getAmbientTemperature() const
{
	return m_area->m_ambiantSurfaceTemperature;
}
std::string Block::toS() const
{
	std::string materialName = isSolid() ? getSolidMaterial().name: "empty";
	std::string output;
	output.reserve(materialName.size() + 16 + (m_fluids.size() * 12));
	output = std::to_string(m_x) + ", " + std::to_string(m_y) + ", " + std::to_string(m_z) + ": " + materialName + ";";
	for(auto& [fluidType, pair] : m_fluids)
		output += fluidType->name + ":" + std::to_string(pair.first) + ";";
	for(auto& pair : m_actors)
		output += ',' + pair.first->m_name;
	return output + describeFeatures() + describePlants();
}
Actor::Actor(Block& l, const Shape& s, const MoveType& mt) : BaseActor<Actor, Block, MoveType, FluidType>(l, s, mt), m_onTaskComplete(nullptr) {}
Actor::Actor(const Shape& s, const MoveType& mt) : BaseActor<Actor, Block, MoveType, FluidType>(s, mt), m_onTaskComplete(nullptr) {}
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
void Actor::doFall(uint32_t distance, Block& block)
{
	(void)distance;
	(void)block;
}
// Take temperature damage, get wet, get dirty, etc.
void Actor::exposedToFluid(const FluidType& fluidType)
{
	(void)fluidType;
}
bool Actor::canSee(const Actor& actor) const
{
	(void)actor;
	return true;
}
void Area::rain(const FluidType& fluidType, uint32_t duration)
{
	m_currentlyRainingFluidType = &fluidType;
	for(Plant& plant : m_plants)
		if(plant.m_plantType.fluidType == fluidType && plant.m_location.m_exposedToSky)
			plant.setHasFluidForNow();
	uint32_t step = s_step + duration;
	std::unique_ptr<ScheduledEvent> event = std::make_unique<RainStopEvent>(step, *this);
	m_rainStopEvent = event.get();
	m_eventSchedule.schedule(std::move(event));
}
void Area::setHour(uint32_t hour, uint32_t dayOfYear)
{
	setAmbientTemperatureFor(hour, dayOfYear);
}
void Area::setDayOfYear(uint32_t dayOfYear)
{
	setAmbientTemperatureFor(0, dayOfYear);
	for(Plant& plant : m_plants)
		plant.setDayOfYear(dayOfYear);
}
void Area::setAmbientTemperatureFor(uint32_t hour, uint32_t dayOfYear)
{
	static uint32_t yearlyHottestDailyAverage = 280;
	static uint32_t yearlyColdestDailyAverage = 256;
	static uint32_t dayOfYearOfSolstice = Config::daysPerYear / 2;
	uint32_t daysFromSolstice = std::abs((int32_t)dayOfYear - (int32_t)dayOfYearOfSolstice);
	uint32_t dailyAverage = yearlyColdestDailyAverage + ((yearlyHottestDailyAverage - yearlyColdestDailyAverage) * (dayOfYearOfSolstice - daysFromSolstice)) / dayOfYearOfSolstice;
	static uint32_t maxDailySwing = 35;
	static uint32_t hottestHourOfDay = 14;
	uint32_t hoursFromHottestHourOfDay = std::abs((int32_t)hottestHourOfDay - (int32_t)hour);
	setAmbientTemperature(dailyAverage + ((maxDailySwing * hoursFromHottestHourOfDay) / (Config::hoursPerDay / 2)) - (maxDailySwing / 2));
}
void Area::setAmbientTemperature(uint32_t temperature)
{
	for(Plant& plant : m_plants)
		if(plant.m_location.m_temperatureZone == TemperatureZone::Surface)
		{
			uint32_t oldTemperature = m_ambiantSurfaceTemperature + plant.m_location.m_deltaTemperature;
			uint32_t newTemperature = temperature + plant.m_location.m_deltaTemperature;
			plant.applyTemperatureChange(oldTemperature, newTemperature);
		}
	m_ambiantSurfaceTemperature = temperature;
}
// Tell the player that an attempted pathing operation is not possible.
void Area::notifyNoRouteFound(Actor& actor) 
{ 
	std::cout << "route not found for " << actor.m_name << " to destination " + actor.m_destination->toS() << "from " << actor.m_location->toS() << std::endl;
}
void Plant::onDie()
{
	m_location.m_plants.remove(this);
	m_location.m_area->m_plants.remove_if([&](Plant& plant){ return &plant == this; });
}
void Plant::onEndOfHarvest()
{
	// Possibly spawn offspring here.
}
bool Animal::canEat(const Plant& plant) const
{
	// TODO: small animals can't eat tree trunks
	return plant.m_plantType.fluidType == m_animalType.fluidType && m_animalType.herbavore;
} 
bool Animal::canEat(const Actor& actor) const
{
	if(!actor.isAlive() && s_step - actor.m_diedAt >= Const::stepTillDecay && !m_animalType.scavenger)
		return false;
	return actor.m_plantType.fluidType == m_animalType.fluidType && m_animalType.carnivore;
}
uint32_t Animal::combatScore() const
{
	uint32_t output = m_mass;
	if(m_animalType.carnivore)
		output *= Const::carnivoreCombatScoreAdjustment;
	return output;
}
bool Animal::willHunt(const Actor& actor) const
{
	assert(m_hungerEvent != nullptr);
	if(!canEat(actor))
		return false;
	// When hunger is 0 score is also 0, when hunger is 100 combat score is multiplyed by 1.58.
	uint32_t score = combatScore() * pow((m_hungerEvent.percentComplete(), 1.1) / 100.f);
	return actor.combatScore() < score;
}
void Animal::onDeath() const
{

}
// Test helpers.
void setSolidLayer(Area& area, uint32_t z, const MaterialType& materialType)
{
	for(uint32_t x = 0; x != area.m_sizeX; ++x)
		for(uint32_t y = 0; y != area.m_sizeY; ++y)
			area.m_blocks[x][y][z].setSolid(materialType);
}
void setSolidLayers(Area& area, uint32_t zbegin, uint32_t zend, const MaterialType& materialType)
{
	for(;zbegin <= zend; ++zbegin)
		setSolidLayer(area, zbegin, materialType);
}
void setSolidWalls(Area& area, uint32_t height, const MaterialType& materialType)
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
void setFullFluidCuboid(Block& low, Block& high, const FluidType& fluidType)
{
	assert(low.m_totalFluidVolume == 0);
	assert(low.fluidCanEnterEver());
	assert(high.m_totalFluidVolume == 0);
	assert(high.fluidCanEnterEver());
	BaseCuboid<Block> cuboid(high, low);
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
				for(auto& [fluidType, pair] : area.m_blocks[x][y][z].m_fluids)
					assert(pair.second->m_fluidType == *fluidType);
}
// Get one fluid group with the specified type. Return null if there is more then one.
FluidGroup* getFluidGroup(Area& area, const FluidType& fluidType)
{
	FluidGroup<Block, Area, FluidType>* output = nullptr;
	for(FluidGroup<Block, Area, FluidType>& fluidGroup : area.m_fluidGroups)
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
