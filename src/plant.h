#pragma once

#include "eventSchedule.h"
#include "util.h"
#include "reservable.h"

class Block;
class PlantGrowthEvent;
class PlantFluidEvent;
class PlantFoliageGrowthEvent;
class PlantEndOfHarvestEvent;
class PlantTemperatureEvent;

struct HarvestData
{
	const uint32_t dayOfYearToStart;
	const uint32_t daysDuration;
	const uint32_t itemQuantity;
	const ItemType& fruitItemType;
	// Infastructure.
	bool operator==(const HarvestData& harvestData){ return this == &harvestData; }
	static std::vector<HarvestData> data;
};
struct PlantSpecies
{
	const std::string name;
	const bool annual;
	const uint32_t maximumGrowingTemperature;
	const uint32_t minimumGrowingTemperature;
	const uint32_t stepsTillDieFromTemperature;
	const uint32_t stepsNeedsFluidFrequency;
	const uint32_t stepsTillDieWithoutFluid;
	const uint32_t stepsTillFullyGrown;
	const bool growsInSunLight;
	const uint32_t rootRangeMax;
	const uint32_t rootRangeMin;
	const uint32_t adultMass;
	const FluidType& fluidType;
	const HarvestData* harvestData;
	// Infastructure.
	bool operator==(const PlantSpecies& plantSpecies){ return this == &plantSpecies; }
	static std::vector<PlantSpecies> data;
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
	bool m_hasFluid;
	uint32_t m_percentFoliage;
	//TODO: Set max reservations to 1 to start, maybe increase later with size?
	Reservable m_reservable;

	Plant(Block& l, const PlantSpecies& pt, uint32_t pg = 0) : m_location(l), m_fluidSource(nullptr), m_plantSpecies(pt), m_percentGrown(pg), m_quantityToHarvest(0), m_hasFluid(true), m_percentFoliage(100), m_reservable(1) { }
	void die();
	void applyTemperatureChange(uint32_t oldTemperature, uint32_t newTemperature);
	void setHasFluidForNow();
	void setMaybeNeedsFluid();
	bool hasFluidSource() const;
	void setDayOfYear(uint32_t dayOfYear);
	void setQuantityToHarvest();
	void harvest(uint32_t quantity);
	void endOfHarvest();
	void updateGrowingStatus();
	uint32_t getGrowthPercent() const;
	uint32_t getRootRange() const;
	uint32_t getPercentFoliage() const;
	uint32_t getFoliageMass() const;
	void removeFoliageMass(uint32_t mass);
	void makeFoliageGrowthEvent();
	void foliageGrowth();
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
	PlantEndOfHarvestEvent(uint32_t step, Plant& p) : ScheduledEventWithPercent(step), m_plant(p) {}
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
// To be used by block.
class HasPlant
{
	Block& m_block;
	Plant* m_plant;
public:
	HasPlant(Block& b);
	void addPlant(const PlantSpecies& plantSpecies, uint32_t growthPercent = 0);
	void updateGrowingStatus();
	void clearPointer();
	Plant& get();
};
