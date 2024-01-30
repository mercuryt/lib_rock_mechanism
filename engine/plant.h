#pragma once

#include "deserializationMemo.h"
#include "reservable.h"
#include "eventSchedule.hpp"
#include "hasShape.h"

#include <algorithm>
#include <iostream>

class Block;

#include <algorithm>

class Block;
class PlantGrowthEvent;
class PlantShapeGrowthEvent;
class PlantFluidEvent;
class PlantFoliageGrowthEvent;
class PlantEndOfHarvestEvent;
class PlantTemperatureEvent;
struct ItemType;
struct FluidType;
struct MaterialType;

struct HarvestData final
{
	const uint16_t dayOfYearToStart;
	const Step stepsDuration;
	const uint32_t itemQuantity;
	const ItemType& fruitItemType;
	// Infastructure.
	bool operator==(const HarvestData& harvestData) const { return this == &harvestData; }
	inline static std::vector<HarvestData> data;
};
struct PlantSpecies final
{
	const std::string name;
	const bool annual;
	const Temperature maximumGrowingTemperature;
	const Temperature minimumGrowingTemperature;
	const Temperature stepsTillDieFromTemperature;
	const Step stepsNeedsFluidFrequency;
	const Volume volumeFluidConsumed;
	const Step stepsTillDieWithoutFluid;
	const Step stepsTillFullyGrown;
	const Step stepsTillFoliageGrowsFromZero;
	const bool growsInSunLight;
	const uint32_t rootRangeMax;
	const uint32_t rootRangeMin;
	const Mass adultMass;
	const uint16_t dayOfYearForSowStart;
	const uint16_t dayOfYearForSowEnd;
	const bool isTree;
	const uint32_t logsGeneratedByFellingWhenFullGrown;
	const uint32_t branchesGeneratedByFellingWhenFullGrown;
	const uint8_t maxWildGrowth;
	const FluidType& fluidType;
	const MaterialType* woodType;
	const HarvestData* harvestData;
	std::vector<const Shape*> shapes;
	// returns base shape and wild growth steps.
	const std::pair<const Shape*, uint8_t> shapeAndWildGrowthForPercentGrown(Percent percentGrown) const;
	const Shape& shapeForPercentGrown(Percent percentGrown) const { return *shapeAndWildGrowthForPercentGrown(percentGrown).first; }
	uint8_t wildGrowthForPercentGrown(Percent percentGrown) const { return shapeAndWildGrowthForPercentGrown(percentGrown).second; }
	// Infastructure.
	bool operator==(const PlantSpecies& plantSpecies) const { return this == &plantSpecies; }
	inline static std::vector<PlantSpecies> data;
	static const PlantSpecies& byName(std::string name)
	{
		auto found = std::ranges::find(data, name, &PlantSpecies::name);
		assert(found != data.end());
		return *found;
	}
};
inline void to_json(Json& data, const PlantSpecies* const& plantSpecies){ data = plantSpecies->name; }
inline void to_json(Json& data, const PlantSpecies& plantSpecies){ data = plantSpecies.name; }
inline void from_json(const Json& data, const PlantSpecies*& plantSpecies){ plantSpecies = &PlantSpecies::byName(data.get<std::string>()); }
class Plant final : public HasShape
{
public:
	Block* m_fluidSource;
	const PlantSpecies& m_plantSpecies;
	HasScheduledEvent<PlantGrowthEvent> m_growthEvent;
	HasScheduledEvent<PlantShapeGrowthEvent> m_shapeGrowthEvent;
	HasScheduledEvent<PlantFluidEvent> m_fluidEvent;
	HasScheduledEvent<PlantTemperatureEvent> m_temperatureEvent;
	HasScheduledEvent<PlantEndOfHarvestEvent> m_endOfHarvestEvent;
	HasScheduledEvent<PlantFoliageGrowthEvent> m_foliageGrowthEvent;
	Percent m_percentGrown;
	uint32_t m_quantityToHarvest;
	Percent m_percentFoliage;
	//TODO: Set max reservations to 1 to start, maybe increase later with size?
	Reservable m_reservable;
	Volume m_volumeFluidRequested;
	uint8_t m_wildGrowth;

	// Shape is passed as a pointer because it may be null, in which case the shape from the species for the given percentGrowth should be used.
	Plant(Block& location, const PlantSpecies& ps, const Shape* shape = nullptr, Percent pg = 0, Volume volumeFluidRequested = 0, Step needsFluidEventStart = 0, bool temperatureIsUnsafe = 0, Step unsafeTemperatureEventStart = 0, uint32_t harvestableQuantity = 0, Percent percentFoliage = 100);
	Plant(const Json& data, DeserializationMemo& deserializationMemo, Block& location);
	Plant(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void die();
	void setTemperature(Temperature temperature);
	void setHasFluidForNow();
	void setMaybeNeedsFluid();
	void addFluid(Volume volume, const FluidType& fluidType);
	void setDayOfYear(uint32_t dayOfYear);
	void setQuantityToHarvest();
	void harvest(uint32_t quantity);
	void endOfHarvest();
	void updateGrowingStatus();
	void removeFoliageMass(Mass mass);
	void doWildGrowth(uint8_t count = 1);
	Mass getFruitMass() const;
	void removeFruitQuantity(uint32_t quantity);
	void makeFoliageGrowthEvent();
	void foliageGrowth();
	void updateShape();
	bool hasFluidSource();
	Percent getGrowthPercent() const;
	uint32_t getRootRange() const;
	Percent getPercentFoliage() const;
	uint32_t getFoliageMass() const;
	Step getStepAtWhichPlantWillDieFromLackOfFluid() const;
	Volume getFluidDrinkVolume() const;
	Step stepsPerShapeChange() const;
	bool readyToHarvest() const { return m_quantityToHarvest != 0; }
	const Volume& getVolumeFluidRequested() { return m_volumeFluidRequested; }
	bool operator==(const Plant& p) const { return &p == this; }
	// Virtual methods.
	void setLocation(Block& block);
	void exit();
	bool isItem() const { return false; }
	bool isActor() const { return false; }
	bool isGeneric() const { assert(false); }
	uint32_t getMass() const { assert(false); }
	uint32_t getVolume() const { assert(false); }
	const MoveType& getMoveType() const { assert(false); }
	uint32_t singleUnitMass() const { return getMass(); }
	void log() const { std::cout << m_plantSpecies.name << std::to_string(getGrowthPercent()); }
};
void to_json(Json& data, const Plant* const& plant);
class PlantGrowthEvent final : public ScheduledEvent
{
	Plant& m_plant;
public:
	PlantGrowthEvent(const Step delay, Plant& p, Step start = 0);
	void execute()
	{
		m_plant.m_percentGrown = 100;
		if(m_plant.m_plantSpecies.annual)
			m_plant.setQuantityToHarvest();
	}
	void clearReferences() { m_plant.m_growthEvent.clearPointer(); }
};
class PlantShapeGrowthEvent final : public ScheduledEvent
{
	Plant& m_plant;
public:
	PlantShapeGrowthEvent(const Step delay, Plant& p, Step start = 0);
	void execute()
	{
		m_plant.updateShape();
		if(m_plant.getGrowthPercent() != 100)
			m_plant.m_shapeGrowthEvent.schedule(m_plant.stepsPerShapeChange(), m_plant);
	}
	void clearReferences() { m_plant.m_shapeGrowthEvent.clearPointer(); }
};
class PlantFoliageGrowthEvent final : public ScheduledEvent
{
	Plant& m_plant;
	public:
	PlantFoliageGrowthEvent(const Step delay, Plant& p, Step start = 0);
	void execute(){ m_plant.foliageGrowth(); }
	void clearReferences(){ m_plant.m_foliageGrowthEvent.clearPointer(); }
};
class PlantEndOfHarvestEvent final : public ScheduledEvent
{
	Plant& m_plant;
	public:
	PlantEndOfHarvestEvent(const Step delay, Plant& p, Step start = 0);
	void execute() { m_plant.endOfHarvest(); }
	void clearReferences() { m_plant.m_endOfHarvestEvent.clearPointer(); }
};
class PlantFluidEvent final : public ScheduledEvent
{
	Plant& m_plant;
	public:
	PlantFluidEvent(const Step delay, Plant& p, Step start = 0);
	void execute() { m_plant.setMaybeNeedsFluid(); }
	void clearReferences() { m_plant.m_fluidEvent.clearPointer(); }
};
class PlantTemperatureEvent final : public ScheduledEvent
{
	Plant& m_plant;
public:
	PlantTemperatureEvent(const Step delay, Plant& p, Step start = 0);
	void execute() { m_plant.die(); }
	void clearReferences() { m_plant.m_temperatureEvent.clearPointer(); }
};
// To be used by Block.
class HasPlant final 
{
	Block& m_block;
	Plant* m_plant;
public:
	HasPlant(Block& b) : m_block(b), m_plant(nullptr) { }
	void createPlant(const PlantSpecies& plantSpecies, Percent growthPercent = 0);
	void updateGrowingStatus();
	void clearPointer();
	void setTemperature(Temperature temperature);
	void erase();
	void set(Plant& plant) { assert(!m_plant); m_plant = &plant; }
	Plant& get() { assert(m_plant); return *m_plant; }
	const Plant& get() const { assert(m_plant); return *m_plant; }
	bool canGrowHereCurrently(const PlantSpecies& plantSpecies) const;
	bool canGrowHereAtSomePointToday(const PlantSpecies& plantSpecies) const;
	bool canGrowHereEver(const PlantSpecies& plantSpecies) const;
	bool anythingCanGrowHereEver() const;
	bool exists() const { return m_plant != nullptr; }
};
// To be used by Area.
class HasPlants final
{
	std::list<Plant> m_plants;
	std::unordered_set<Plant*> m_plantsOnSurface;
public:
	Plant& emplace(Block& location, const PlantSpecies& species, const Shape* shape = nullptr, Percent percentGrowth = 0, Volume volumeFluidRequested = 0, Step needsFluidEventStart = 0, bool temperatureIsUnsafe = 0, Step unsafeTemperatureEventStart = 0, uint32_t harvestableQuantity = 0, Percent percentFoliage = 100);
	Plant& emplace(const Json& data, DeserializationMemo& deserializationMemo);
	void erase(Plant& plant);
	void onChangeAmbiantSurfaceTemperature();
	std::unordered_set<Plant*>& getPlantsOnSurface() { return m_plantsOnSurface; }
	std::list<Plant>& getAll() { return m_plants; }
	const std::list<Plant>& getAllConst() const { return m_plants; }
};
