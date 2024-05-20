#pragma once

#include "reservable.h"
#include "eventSchedule.hpp"
#include "hasShape.h"
#include "types.h"

#include <algorithm>
#include <iostream>

class Block;
struct DeserializationMemo;

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
	const ItemType& fruitItemType;
	const Step stepsDuration;
	const uint32_t itemQuantity;
	const uint16_t dayOfYearToStart;
	HarvestData(const uint16_t doyts, const Step sd, const uint32_t iq, const ItemType& fit) :
		fruitItemType(fit), stepsDuration(sd), itemQuantity(iq), dayOfYearToStart(doyts) { }
	// Infastructure.
	[[nodiscard]] bool operator==(const HarvestData& harvestData) const { return this == &harvestData; }
};
inline std::deque<HarvestData> harvestDataStore;
struct PlantSpecies final
{
	std::vector<const Shape*> shapes;
	const std::string name;
	const FluidType& fluidType;
	const MaterialType* woodType = nullptr;
	const HarvestData* harvestData = nullptr;
	const Step stepsNeedsFluidFrequency = 0;
	const Step stepsTillDieWithoutFluid = 0;
	const Step stepsTillFullyGrown = 0;
	const Step stepsTillFoliageGrowsFromZero = 0;
	const Step stepsTillDieFromTemperature = 0;
	const DistanceInBlocks rootRangeMax = 0;
	const uint32_t rootRangeMin = 0;
	const uint32_t logsGeneratedByFellingWhenFullGrown = 0;
	const uint32_t branchesGeneratedByFellingWhenFullGrown = 0;
	const Mass adultMass = 0;
	const Temperature maximumGrowingTemperature = 0;
	const Temperature minimumGrowingTemperature = 0;
	const Volume volumeFluidConsumed = 0;
	const uint16_t dayOfYearForSowStart = 0;
	const uint16_t dayOfYearForSowEnd = 0;
	const uint8_t maxWildGrowth = 0;
	const bool annual = false;
	const bool growsInSunLight = false;
	const bool isTree = false;
	// returns base shape and wild growth steps.
	[[nodiscard]] const std::pair<const Shape*, uint8_t> shapeAndWildGrowthForPercentGrown(Percent percentGrown) const;
	[[nodiscard]] const Shape& shapeForPercentGrown(Percent percentGrown) const { return *shapeAndWildGrowthForPercentGrown(percentGrown).first; }
	[[nodiscard]] uint8_t wildGrowthForPercentGrown(Percent percentGrown) const { return shapeAndWildGrowthForPercentGrown(percentGrown).second; }
	PlantSpecies(std::string n, const bool a, const Temperature magt, const Temperature migt, const Temperature stdft, const Step snff, const Volume vfc, const Step stdwf, const Step stfg, const Step stfgfz, const bool gisl, const DistanceInBlocks rrma, const DistanceInBlocks rrmi, const Mass am, const uint16_t doyfss, const uint16_t doyfse, const bool it, const uint32_t lgfwfg, const uint32_t bgfwfg, const uint8_t mwg, const FluidType& ft, const MaterialType* wt) :
		name(n), fluidType(ft), woodType(wt), stepsNeedsFluidFrequency(snff), stepsTillDieWithoutFluid(stdwf), 
		stepsTillFullyGrown(stfg), stepsTillFoliageGrowsFromZero(stfgfz), stepsTillDieFromTemperature(stdft), 
		rootRangeMax(rrma), rootRangeMin(rrmi), logsGeneratedByFellingWhenFullGrown(lgfwfg), branchesGeneratedByFellingWhenFullGrown(bgfwfg), 
		adultMass(am), maximumGrowingTemperature(magt), minimumGrowingTemperature(migt), 
		volumeFluidConsumed(vfc), dayOfYearForSowStart(doyfss), dayOfYearForSowEnd(doyfse), maxWildGrowth(mwg), annual(a), growsInSunLight(gisl), 
		isTree(it) { }
	// Infastructure.
	[[nodiscard]] bool operator==(const PlantSpecies& plantSpecies) const { return this == &plantSpecies; }
	[[nodiscard]] static const PlantSpecies& byName(std::string name);
};
inline std::vector<PlantSpecies> plantSpeciesDataStore;
inline void to_json(Json& data, const PlantSpecies* const& plantSpecies){ data = plantSpecies->name; }
inline void to_json(Json& data, const PlantSpecies& plantSpecies){ data = plantSpecies.name; }
inline void from_json(const Json& data, const PlantSpecies*& plantSpecies){ plantSpecies = &PlantSpecies::byName(data.get<std::string>()); }
class Plant final : public HasShape
{
public:
	Reservable m_reservable;
	HasScheduledEvent<PlantGrowthEvent> m_growthEvent;
	HasScheduledEvent<PlantShapeGrowthEvent> m_shapeGrowthEvent;
	HasScheduledEvent<PlantFluidEvent> m_fluidEvent;
	HasScheduledEvent<PlantTemperatureEvent> m_temperatureEvent;
	HasScheduledEvent<PlantEndOfHarvestEvent> m_endOfHarvestEvent;
	HasScheduledEvent<PlantFoliageGrowthEvent> m_foliageGrowthEvent;
	Block* m_fluidSource = nullptr;
	const PlantSpecies& m_plantSpecies;
	Quantity m_quantityToHarvest = 0;
	Percent m_percentGrown = 0;
	Percent m_percentFoliage = 0;
	//TODO: Set max reservations to 1 to start, maybe increase later with size?
	Volume m_volumeFluidRequested = 0;
	uint8_t m_wildGrowth = 0;

	// Shape is passed as a pointer because it may be null, in which case the shape from the species for the given percentGrowth should be used.
	Plant(Block& location, const PlantSpecies& ps, const Shape* shape = nullptr, Percent pg = 0, Volume volumeFluidRequested = 0, Step needsFluidEventStart = 0, bool temperatureIsUnsafe = 0, Step unsafeTemperatureEventStart = 0, uint32_t harvestableQuantity = 0, Percent percentFoliage = 100);
	Plant(const Json& data, DeserializationMemo& deserializationMemo, Block& location);
	Plant(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void die();
	void remove() { die(); } // TODO: seperate remove from die when corpse is added.
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
	void removeFruitQuantity(uint32_t quantity);
	void makeFoliageGrowthEvent();
	void foliageGrowth();
	void updateShape();
	void setLocation(Block& block);
	void exit();
	[[nodiscard]] Mass getFruitMass() const;
	[[nodiscard]] bool hasFluidSource();
	[[nodiscard]] Percent getGrowthPercent() const;
	[[nodiscard]] uint32_t getRootRange() const;
	[[nodiscard]] Percent getPercentFoliage() const;
	[[nodiscard]] uint32_t getFoliageMass() const;
	[[nodiscard]] Step getStepAtWhichPlantWillDieFromLackOfFluid() const;
	[[nodiscard]] Volume getFluidDrinkVolume() const;
	[[nodiscard]] Step stepsPerShapeChange() const;
	[[nodiscard]] bool readyToHarvest() const { return m_quantityToHarvest != 0; }
	[[nodiscard]] const Volume& getVolumeFluidRequested() { return m_volumeFluidRequested; }
	[[nodiscard]] bool operator==(const Plant& p) const { return &p == this; }
	// Virtual methods.
	[[nodiscard]] bool isItem() const { return false; }
	[[nodiscard]] bool isActor() const { return false; }
	[[nodiscard]] bool isGeneric() const { assert(false); return false;}
	[[nodiscard]] uint32_t getMass() const { assert(false); return 0;}
	[[nodiscard]] uint32_t getVolume() const { assert(false); return 0; }
	[[nodiscard]] const MoveType& getMoveType() const;
	[[nodiscard]] uint32_t singleUnitMass() const { return getMass(); }
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
class AreaHasPlants final
{
	std::list<Plant> m_plants;
	std::unordered_set<Plant*> m_plantsOnSurface;
public:
	Plant& emplace(Block& location, const PlantSpecies& species, const Shape* shape = nullptr, Percent percentGrowth = 0, Volume volumeFluidRequested = 0, Step needsFluidEventStart = 0, bool temperatureIsUnsafe = 0, Step unsafeTemperatureEventStart = 0, uint32_t harvestableQuantity = 0, Percent percentFoliage = 100);
	Plant& emplace(const Json& data, DeserializationMemo& deserializationMemo);
	void erase(Plant& plant);
	void onChangeAmbiantSurfaceTemperature();
	[[nodiscard]] std::unordered_set<Plant*>& getPlantsOnSurface() { return m_plantsOnSurface; }
	[[nodiscard]] std::list<Plant>& getAll() { return m_plants; }
	[[nodiscard]] const std::list<Plant>& getAllConst() const { return m_plants; }
};
