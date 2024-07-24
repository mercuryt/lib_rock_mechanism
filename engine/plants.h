#pragma once

#include "deserializationMemo.h"
#include "eventSchedule.hpp"
#include "hasShapes.h"
#include "plantSpecies.h"
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
	DataVector<const PlantSpecies*, PlantIndex> m_species;
	DataVector<BlockIndex, PlantIndex> m_fluidSource;
	DataVector<Quantity, PlantIndex> m_quantityToHarvest;
	DataVector<Percent, PlantIndex> m_percentGrown;
	DataVector<Percent, PlantIndex> m_percentFoliage;
	DataVector<uint8_t, PlantIndex> m_wildGrowth;
	DataVector<CollisionVolume, PlantIndex> m_volumeFluidRequested;
	void resize(HasShapeIndex newSize);
	void moveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex);
	void updateFluidVolumeRequested(PlantIndex index);
public:
	Plants(Area& area);
	void load(const Json& data);
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
	void fromJson(Plants& plants, Area& area, const Json& data);
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
	[[nodiscard]] Quantity getQuantityToHarvest(PlantIndex index) const { return m_quantityToHarvest.at(index); }
	[[nodiscard]] Step stepsPerShapeChange(PlantIndex index) const;
	[[nodiscard]] bool readyToHarvest(PlantIndex index) const { return m_quantityToHarvest.at(index) != 0; }
	[[nodiscard]] const CollisionVolume& getVolumeFluidRequested(PlantIndex index) const { return m_volumeFluidRequested.at(index); }
	[[nodiscard]] bool temperatureEventExists(PlantIndex index) const;
	[[nodiscard]] Json toJson() const;
	void log(PlantIndex index) const;
	friend class PlantGrowthEvent;
	friend class PlantShapeGrowthEvent;
	friend class PlantFoliageGrowthEvent;
	friend class PlantEndOfHarvestEvent;
	friend class PlantFluidEvent;
	friend class PlantTemperatureEvent;
	Plants(Plants&) = delete;
	Plants(Plants&&) = delete;
};
void to_json(Json& data, const Plants& plants);
class PlantGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	PlantGrowthEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_plant == oldIndex); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantShapeGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantShapeGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	PlantShapeGrowthEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_plant == oldIndex); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantFoliageGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantFoliageGrowthEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	PlantFoliageGrowthEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_plant == oldIndex); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantEndOfHarvestEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantEndOfHarvestEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	PlantEndOfHarvestEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_plant == oldIndex); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantFluidEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantFluidEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	PlantFluidEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_plant == oldIndex); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantTemperatureEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantTemperatureEvent(Area& area, const Step delay, PlantIndex p, Step start = 0);
	PlantTemperatureEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex(HasShapeIndex oldIndex, HasShapeIndex newIndex) { assert(m_plant == oldIndex); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
