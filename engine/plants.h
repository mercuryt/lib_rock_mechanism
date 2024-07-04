#pragma once

#include "deserializationMemo.h"
#include "eventSchedule.hpp"
#include "hasShapes.h"
#include "types.h"

#include <vector>

struct PlantSpecies;
struct FluidType;
class PlantGrowthEvent;
class PlantShapeGrowthEvent;
class PlantFluidEvent;
class PlantTemperatureEvent;
class PlantEndOfHarvestEvent;
class PlantFoliageGrowthEvent;
class Simulation;

struct PlantParamaters
{
	BlockIndex location = BLOCK_INDEX_MAX;
	const PlantSpecies& species;
	const Shape* shape = nullptr;
	Percent percentGrown = 0;
	Percent percentFoliage = 0;
	uint8_t wildGrowth = 0;
	Quantity quantityToHarvest = 0;
	Percent percentNeedsFluid = 0;
	Percent percentNeedsSafeTemperature = 0;
	Percent percentFoliageGrowth = 0;
};
class Plants final : public HasShapes
{
	HasScheduledEvents<PlantGrowthEvent> m_growthEvent;
	//TODO: Why is this a seperate event from plant growth?
	HasScheduledEvents<PlantShapeGrowthEvent> m_shapeGrowthEvent;
	HasScheduledEvents<PlantFluidEvent> m_fluidEvent;
	HasScheduledEvents<PlantTemperatureEvent> m_temperatureEvent;
	HasScheduledEvents<PlantEndOfHarvestEvent> m_endOfHarvestEvent;
	HasScheduledEvents<PlantFoliageGrowthEvent> m_foliageGrowthEvent;
	std::vector<const PlantSpecies*> m_species;
	std::vector<BlockIndex> m_fluidSource;
	std::vector<Quantity> m_quantityToHarvest;
	std::vector<Percent> m_percentGrown;
	std::vector<Percent> m_percentFoliage;
	std::vector<uint8_t> m_wildGrowth;
	std::vector<CollisionVolume> m_volumeFluidRequested;
	void resize(HasShapeIndex newSize);
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	[[nodiscard]] bool indexCanBeMoved(HasShapeIndex) const { return true; }
public:
	Plants(Area& area);
	Plants(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void onChangeAmbiantSurfaceTemperature();
	PlantIndex create(PlantParamaters paramaters);
	void destroy(PlantIndex index);
	void die(PlantIndex index);
	void remove(PlantIndex index) { die(index); } // TODO: seperate remove from die when corpse is added.
	void setTemperature(PlantIndex index, Temperature temperature);
	void setHasFluidForNow(PlantIndex index);
	void setMaybeNeedsFluid(PlantIndex index);
	void addFluid(PlantIndex index, Volume volume, const FluidType& fluidType);
	void setDayOfYear(PlantIndex index, uint32_t dayOfYear);
	void setQuantityToHarvest(PlantIndex index);
	void harvest(PlantIndex index, uint32_t quantity);
	void endOfHarvest(PlantIndex index);
	void updateGrowingStatus(PlantIndex index);
	void removeFoliageMass(PlantIndex index, Mass mass);
	void doWildGrowth(PlantIndex index, uint8_t count = 1);
	void removeFruitQuantity(PlantIndex index, uint32_t quantity);
	void makeFoliageGrowthEvent(PlantIndex index);
	void foliageGrowth(PlantIndex index);
	void updateShape(PlantIndex index);
	void setLocation(BlockIndex block, Area* area);
	void exit(PlantIndex index);
	[[nodiscard]] const PlantSpecies& getSpecies(PlantIndex index) const;
	[[nodiscard]] Mass getFruitMass(PlantIndex index) const;
	// Not const: updates cache.
	[[nodiscard]] bool hasFluidSource(PlantIndex index);
	[[nodiscard]] bool isGrowing(PlantIndex index) const { return m_growthEvent.exists(index); }
	[[nodiscard]] Percent getPercentGrown(PlantIndex index) const;
	[[nodiscard]] uint32_t getRootRange(PlantIndex index) const;
	[[nodiscard]] Percent getPercentFoliage(PlantIndex index) const;
	[[nodiscard]] uint32_t getFoliageMass(PlantIndex index) const;
	[[nodiscard]] Step getStepAtWhichPlantWillDieFromLackOfFluid(PlantIndex index) const;
	[[nodiscard]] Volume getFluidDrinkVolume(PlantIndex index) const;
	[[nodiscard]] Quantity getQuantityToHarvest(PlantIndex index) const { return m_quantityToHarvest.at(index); }
	[[nodiscard]] Step stepsPerShapeChange(PlantIndex index) const;
	[[nodiscard]] bool readyToHarvest(PlantIndex index) const { return m_quantityToHarvest.at(index) != 0; }
	[[nodiscard]] const Volume& getVolumeFluidRequested(PlantIndex index) const { return m_volumeFluidRequested.at(index); }
	void log(PlantIndex index) const;
	friend class PlantGrowthEvent;
	friend class PlantShapeGrowthEvent;
	friend class PlantFoliageGrowthEvent;
	friend class PlantEndOfHarvestEvent;
	friend class PlantFluidEvent;
	friend class PlantTemperatureEvent;
};
class PlantGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
};
class PlantShapeGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantShapeGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
};
class PlantFoliageGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
	public:
	PlantFoliageGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
};
class PlantEndOfHarvestEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantEndOfHarvestEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
};
class PlantFluidEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantFluidEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
};
class PlantTemperatureEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantTemperatureEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
};
