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
	HasScheduledEvents<PlantGrowthEvent> m_growthEvent;
	//TODO: Why is this a seperate event from plant growth?
	HasScheduledEvents<PlantShapeGrowthEvent> m_shapeGrowthEvent;
	HasScheduledEvents<PlantFluidEvent> m_fluidEvent;
	HasScheduledEvents<PlantTemperatureEvent> m_temperatureEvent;
	HasScheduledEvents<PlantEndOfHarvestEvent> m_endOfHarvestEvent;
	HasScheduledEvents<PlantFoliageGrowthEvent> m_foliageGrowthEvent;
	DataVector<PlantSpeciesId, PlantIndex> m_species;
	DataVector<BlockIndex, PlantIndex> m_fluidSource;
	DataVector<Quantity, PlantIndex> m_quantityToHarvest;
	DataVector<Percent, PlantIndex> m_percentGrown;
	DataVector<Percent, PlantIndex> m_percentFoliage;
	DataVector<uint8_t, PlantIndex> m_wildGrowth;
	DataVector<CollisionVolume, PlantIndex> m_volumeFluidRequested;
	void resize(const PlantIndex& newSize);
	void moveIndex(const PlantIndex& oldIndex, const PlantIndex& newIndex);
	void updateFluidVolumeRequested(const PlantIndex& index);
public:
	PlantIndexSet m_onSurface;
	Plants(Area& area);
	void load(const Json& data);
	void onChangeAmbiantSurfaceTemperature();
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
	void setLocation(const PlantIndex& index, const BlockIndex& location, const Facing& facing);
	void exit(const PlantIndex& index);
	void fromJson(Plants& plants, Area& area, const Json& data);
	[[nodiscard]] PlantIndexSet& getOnSurface() { return m_onSurface; }
	[[nodiscard]] bool isOnSurface(const PlantIndex& index) { return m_onSurface.contains(index); }
	[[nodiscard]] bool blockIsFull(const BlockIndex& index);
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
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
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
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
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
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
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
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
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
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
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
	void onMoveIndex(const HasShapeIndex& oldIndex, const HasShapeIndex& newIndex) { assert(m_plant == oldIndex.toPlant()); m_plant = PlantIndex::cast(newIndex); }
	[[nodiscard]] Json toJson() const;
};
