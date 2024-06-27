#pragma once

#include "types.h"
#include "config.h"

#include <deque>

struct ItemType;
struct FluidType;
struct MaterialType;
struct Shape;

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
	//TODO: Paramaterize these arguments.
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
