#pragma once

#include "eventSchedule.h"
#include "reservable.h"
#include "eventSchedule.hpp"

#include <algorithm>

class Block;
class PlantGrowthEvent;
class PlantFluidEvent;
class PlantFoliageGrowthEvent;
class PlantEndOfHarvestEvent;
class PlantTemperatureEvent;
struct ItemType;
struct FluidType;

struct HarvestData
{
	const uint32_t dayOfYearToStart;
	const uint32_t daysDuration;
	const uint32_t itemQuantity;
	const ItemType& fruitItemType;
	// Infastructure.
	bool operator==(const HarvestData& harvestData){ return this == &harvestData; }
	inline static std::vector<HarvestData> data;
};
struct PlantSpecies
{
	const std::string name;
	const bool annual;
	const uint32_t maximumGrowingTemperature;
	const uint32_t minimumGrowingTemperature;
	const uint32_t stepsTillDieFromTemperature;
	const uint32_t stepsNeedsFluidFrequency;
	const uint32_t volumeFluidConsumed;
	const uint32_t stepsTillDieWithoutFluid;
	const uint32_t stepsTillFullyGrown;
	const uint32_t stepsTillFoliageGrowsFromZero;
	const bool growsInSunLight;
	const uint32_t rootRangeMax;
	const uint32_t rootRangeMin;
	const uint32_t adultMass;
	const uint32_t dayOfYearForSowStart;
	const uint32_t dayOfYearForSowEnd;
	const FluidType& fluidType;
	const HarvestData* harvestData;
	// Infastructure.
	bool operator==(const PlantSpecies& plantSpecies){ return this == &plantSpecies; }
	inline static std::vector<PlantSpecies> data;
	static const PlantSpecies& byName(std::string name)
	{
		auto found = std::ranges::find(data, name, &PlantSpecies::name);
		assert(found != data.end());
		return *found;
	}
};
class Plant
{
public:
	Block& m_location;
	Block* m_fluidSource;
	const PlantSpecies& m_plantSpecies;
	HasScheduledEvent<PlantGrowthEvent> m_growthEvent;
	HasScheduledEvent<PlantFluidEvent> m_fluidEvent;
	HasScheduledEvent<PlantTemperatureEvent> m_temperatureEvent;
	HasScheduledEvent<PlantEndOfHarvestEvent> m_endOfHarvestEvent;
	HasScheduledEvent<PlantFoliageGrowthEvent> m_foliageGrowthEvent;
	uint32_t m_percentGrown;
	uint32_t m_quantityToHarvest;
	uint32_t m_percentFoliage;
	//TODO: Set max reservations to 1 to start, maybe increase later with size?
	Reservable m_reservable;
	uint32_t m_volumeFluidRequested;

	Plant(Block& l, const PlantSpecies& ps, uint32_t pg = 0);
	void die();
	void setTemperature(uint32_t temperature);
	void setHasFluidForNow();
	void setMaybeNeedsFluid();
	void addFluid(uint32_t volume, const FluidType& fluidType);
	void setDayOfYear(uint32_t dayOfYear);
	void setQuantityToHarvest();
	void harvest(uint32_t quantity);
	void endOfHarvest();
	void updateGrowingStatus();
	void removeFoliageMass(uint32_t mass);
	uint32_t getFruitMass() const;
	void removeFruitQuantity(uint32_t quantity);
	void makeFoliageGrowthEvent();
	void foliageGrowth();
	bool hasFluidSource();
	uint32_t getGrowthPercent() const;
	uint32_t getRootRange() const;
	uint32_t getPercentFoliage() const;
	uint32_t getFoliageMass() const;
	uint32_t getStepAtWhichPlantWillDieFromLackOfFluid() const;
	uint32_t getFluidDrinkVolume() const;
	bool readyToHarvest() const { return m_quantityToHarvest != 0; }
	const uint32_t& getVolumeFluidRequested();
};

class PlantGrowthEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantGrowthEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute()
	{
		m_plant.m_percentGrown = 100;
		if(m_plant.m_plantSpecies.annual)
			m_plant.setQuantityToHarvest();
	}
	~PlantGrowthEvent() { m_plant.m_growthEvent.clearPointer(); }
};
class PlantFoliageGrowthEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantFoliageGrowthEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute(){ m_plant.foliageGrowth(); }
	~PlantFoliageGrowthEvent(){ m_plant.m_foliageGrowthEvent.clearPointer(); }
};
class PlantEndOfHarvestEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantEndOfHarvestEvent(uint32_t delay, Plant& p) : ScheduledEventWithPercent(delay), m_plant(p) {}
	void execute() { m_plant.endOfHarvest(); }
	~PlantEndOfHarvestEvent() { m_plant.m_endOfHarvestEvent.clearPointer(); }
};
class PlantFluidEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantFluidEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.setMaybeNeedsFluid(); }
	~PlantFluidEvent() { m_plant.m_fluidEvent.clearPointer(); }
};
class PlantTemperatureEvent : public ScheduledEventWithPercent
{
	Plant& m_plant;
public:
	PlantTemperatureEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
	void execute() { m_plant.die(); }
	~PlantTemperatureEvent() { m_plant.m_temperatureEvent.clearPointer(); }
};
// To be used by Block.
class HasPlant
{
	Block& m_block;
	Plant* m_plant;
public:
	HasPlant(Block& b) : m_block(b) { }
	void addPlant(const PlantSpecies& plantSpecies, uint32_t growthPercent = 0);
	void updateGrowingStatus();
	void clearPointer();
	void setTemperature(uint32_t temperature);
	Plant& get();
	bool canGrowHere(const PlantSpecies& plantSpecies) const;
	bool exists() const { return m_plant != nullptr; }
};
// To be used by Area.
class HasPlants
{
	std::list<Plant> m_plants;
	std::list<Plant*> m_plantsOnSurface;
public:
	void emplace(Block& location, const PlantSpecies& species, uint32_t percentGrowth);
	void erase(Plant& plant);
	void onChangeAmbiantSurfaceTemperature();
};
