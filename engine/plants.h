#pragma once

#include "idTypes.h"
#include "index.h"
#include "types.h"
#include "eventSchedule.hpp"
#include "hasShapes.h"

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
struct DeserializationMemo;

struct PlantParamaters
{
	BlockIndex location;
	PlantSpeciesId species;
	ShapeId shape = ShapeId::null();
	Percent percentGrown = Percent::null();
	Percent percentFoliage = Percent::null();
	uint8_t wildGrowth = 0;
	Quantity quantityToHarvest = Quantity::null();
	Percent percentNeedsFluid = Percent::null();
	Percent percentNeedsSafeTemperature = Percent::null();
	Percent percentFoliageGrowth = Percent::null();
	FactionId faction = FactionId::null();
};
class Plants final : public HasShapes<Plants, PlantIndex>
{
	HasScheduledEvents<PlantGrowthEvent, PlantIndex> m_growthEvent;
	//TODO: Why is this a seperate event from plant growth?
	HasScheduledEvents<PlantShapeGrowthEvent, PlantIndex> m_shapeGrowthEvent;
	HasScheduledEvents<PlantFluidEvent, PlantIndex> m_fluidEvent;
	HasScheduledEvents<PlantTemperatureEvent, PlantIndex> m_temperatureEvent;
	HasScheduledEvents<PlantEndOfHarvestEvent, PlantIndex> m_endOfHarvestEvent;
	HasScheduledEvents<PlantFoliageGrowthEvent, PlantIndex> m_foliageGrowthEvent;
	StrongVector<PlantSpeciesId, PlantIndex> m_species;
	StrongVector<BlockIndex, PlantIndex> m_fluidSource;
	StrongVector<Quantity, PlantIndex> m_quantityToHarvest;
	StrongVector<Percent, PlantIndex> m_percentGrown;
	StrongVector<Percent, PlantIndex> m_percentFoliage;
	StrongVector<uint8_t, PlantIndex> m_wildGrowth;
	StrongVector<CollisionVolume, PlantIndex> m_volumeFluidRequested;
	PlantIndex m_incrementalSortPosition;
	std::chrono::microseconds m_averageSortTimePerPlant = std::chrono::microseconds(10);
	uint32_t m_averageSortTimeSampleSize = 0;
	uint8_t m_sortEntropy = 0;
	void moveIndex(const PlantIndex& oldIndex, const PlantIndex& newIndex);
	void updateFluidVolumeRequested(const PlantIndex& index);
public:
	Plants(Area& area);
	void load(const Json& data);
	void onChangeAmbiantSurfaceTemperature();
	template<typename Action>
	void forEachData(Action&& action);
	PlantIndex create(PlantParamaters paramaters);
	void destroy(const PlantIndex& index);
	void die(const PlantIndex& index);
	void remove(const PlantIndex& index) { die(index); } // TODO: seperate remove from die when corpse is added.
	void setTemperature(const PlantIndex& index, const Temperature& temperature);
	void setHasFluidForNow(const PlantIndex& index);
	void setMaybeNeedsFluid(const PlantIndex& index);
	void addFluid(const PlantIndex& index, const CollisionVolume& volume, const FluidTypeId& fluidType);
	void setDayOfYear(const PlantIndex& index, uint32_t dayOfYear);
	void setQuantityToHarvest(const PlantIndex& index);
	void harvest(const PlantIndex& index, const Quantity& quantity);
	void endOfHarvest(const PlantIndex& index);
	void updateGrowingStatus(const PlantIndex& index);
	void removeFoliageMass(const PlantIndex& index, const Mass& mass);
	void doWildGrowth(const PlantIndex& index, uint8_t count = 1);
	void removeFruitQuantity(const PlantIndex& index, const Quantity& quantity);
	void makeFoliageGrowthEvent(const PlantIndex& index);
	void foliageGrowth(const PlantIndex& index);
	void updateShape(const PlantIndex& index);
	void setShape(const PlantIndex& index, const ShapeId& shape);
	void setLocation(const PlantIndex& index, const BlockIndex& location, const Facing4& facing);
	void exit(const PlantIndex& index);
	// Plants are able to do a more efficient range based sort becasue no references to them are ever stored.
	// Portables are required to swap one at a time.
	void sortRange(const PlantIndex& begin, const PlantIndex& end);
	void maybeIncrementalSort(const std::chrono::microseconds timeBugdget);
	[[nodiscard]] bool blockIsFull(const BlockIndex& index) const;
	[[nodiscard]] PlantIndices getAll() const;
	[[nodiscard]] PlantSpeciesId getSpecies(const PlantIndex& index) const;
	[[nodiscard]] Mass getFruitMass(const PlantIndex& index) const;
	// Not const: updates cache.
	[[nodiscard]] bool hasFluidSource(const PlantIndex& index);
	[[nodiscard]] bool isGrowing(const PlantIndex& index) const { return m_growthEvent.exists(index); }
	[[nodiscard]] Percent getPercentGrown(const PlantIndex& index) const;
	[[nodiscard]] DistanceInBlocks getRootRange(const PlantIndex& index) const;
	[[nodiscard]] Percent getPercentFoliage(const PlantIndex& index) const;
	[[nodiscard]] Mass getFoliageMass(const PlantIndex& index) const;
	[[nodiscard]] Step getStepAtWhichPlantWillDieFromLackOfFluid(const PlantIndex& index) const;
	[[nodiscard]] Quantity getQuantityToHarvest(const PlantIndex& index) const { return m_quantityToHarvest[index]; }
	[[nodiscard]] Step stepsPerShapeChange(const PlantIndex& index) const;
	[[nodiscard]] bool readyToHarvest(const PlantIndex& index) const { return m_quantityToHarvest[index] != 0; }
	[[nodiscard]] CollisionVolume getVolumeFluidRequested(const PlantIndex& index) const { return m_volumeFluidRequested[index]; }
	[[nodiscard]] bool temperatureEventExists(const PlantIndex& index) const;
	[[nodiscard]] bool fluidEventExists(const PlantIndex& index) const;
	[[nodiscard]] Percent getFluidEventPercentComplete(const PlantIndex& index) const { return m_fluidEvent.percentComplete(index); }
	[[nodiscard]] bool growthEventExists(const PlantIndex& index) const;
	[[nodiscard]] Json toJson() const;
	void log(const PlantIndex& index) const;
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
	PlantGrowthEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start = Step::null());
	PlantGrowthEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex([[maybe_unused]] const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantShapeGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantShapeGrowthEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start = Step::null());
	PlantShapeGrowthEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex([[maybe_unused]] const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantFoliageGrowthEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantFoliageGrowthEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start = Step::null());
	PlantFoliageGrowthEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex([[maybe_unused]] const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantEndOfHarvestEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantEndOfHarvestEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start = Step::null());
	PlantEndOfHarvestEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex([[maybe_unused]] const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantFluidEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantFluidEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start = Step::null());
	PlantFluidEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex([[maybe_unused]] const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
class PlantTemperatureEvent final : public ScheduledEvent
{
	PlantIndex m_plant;
public:
	PlantTemperatureEvent(Area& area, const Step& delay, const PlantIndex& p, const Step start = Step::null());
	PlantTemperatureEvent(Simulation& simulation, const Json& data);
	void execute(Simulation&, Area* area);
	void clearReferences(Simulation&, Area* area);
	void onMoveIndex([[maybe_unused]] const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
