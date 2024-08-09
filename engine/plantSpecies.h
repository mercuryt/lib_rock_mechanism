#pragma once

#include "dataVector.h"
#include "types.h"
#include "config.h"

struct ItemType;
struct FluidType;
struct MaterialType;

struct PlantSpeciesParamaters final
{
	std::vector<ShapeId> shapes = {};
	const std::string name;
	const FluidTypeId fluidType;
	const MaterialTypeId woodType = MaterialTypeId::null();
	const Step stepsNeedsFluidFrequency = Step::null();
	const Step stepsTillDieWithoutFluid = Step::null();
	const Step stepsTillFullyGrown = Step::null();
	const Step stepsTillFoliageGrowsFromZero = Step::null();
	const Step stepsTillDieFromTemperature = Step::null();
	const DistanceInBlocks rootRangeMax = DistanceInBlocks::null();
	const DistanceInBlocks rootRangeMin = DistanceInBlocks::null();
	const Quantity logsGeneratedByFellingWhenFullGrown = Quantity::null();
	const Quantity branchesGeneratedByFellingWhenFullGrown = Quantity::null();
	const Mass adultMass = Mass::null();
	const Temperature maximumGrowingTemperature = Temperature::null();
	const Temperature minimumGrowingTemperature = Temperature::null();
	const Volume volumeFluidConsumed = Volume::null();
	const uint16_t dayOfYearForSowStart = 0;
	const uint16_t dayOfYearForSowEnd = 0;
	const uint8_t maxWildGrowth = 0;
	const bool annual = false;
	const bool growsInSunLight = false;
	const bool isTree = false;
	const ItemTypeId fruitItemType = ItemTypeId::null();
	const Step stepsDurationHarvest = Step::null();
	const Quantity itemQuantityToHarvest = Quantity::null();
	const uint16_t dayOfYearToStartHarvest = 0;
};
class PlantSpecies final
{
	static PlantSpecies data;
	DataVector<std::vector<ShapeId>, PlantSpeciesId> m_shapes;
	DataVector<std::string, PlantSpeciesId> m_name;
	DataVector<FluidTypeId, PlantSpeciesId> m_fluidType;
	DataVector<MaterialTypeId, PlantSpeciesId> m_woodType;
	DataVector<Step, PlantSpeciesId> m_stepsNeedsFluidFrequency;
	DataVector<Step, PlantSpeciesId> m_stepsTillDieWithoutFluid;
	DataVector<Step, PlantSpeciesId> m_stepsTillFullyGrown;
	DataVector<Step, PlantSpeciesId> m_stepsTillFoliageGrowsFromZero;
	DataVector<Step, PlantSpeciesId> m_stepsTillDieFromTemperature;
	DataVector<DistanceInBlocks, PlantSpeciesId> m_rootRangeMax;
	DataVector<DistanceInBlocks, PlantSpeciesId> m_rootRangeMin;
	DataVector<Quantity, PlantSpeciesId> m_logsGeneratedByFellingWhenFullGrown;
	DataVector<Quantity, PlantSpeciesId> m_branchesGeneratedByFellingWhenFullGrown;
	DataVector<Mass, PlantSpeciesId> m_adultMass;
	DataVector<Temperature, PlantSpeciesId> m_maximumGrowingTemperature;
	DataVector<Temperature, PlantSpeciesId> m_minimumGrowingTemperature;
	DataVector<Volume, PlantSpeciesId> m_volumeFluidConsumed;
	DataVector<uint16_t, PlantSpeciesId> m_dayOfYearForSowStart;
	DataVector<uint16_t, PlantSpeciesId> m_dayOfYearForSowEnd;
	DataVector<uint8_t, PlantSpeciesId> m_maxWildGrowth;
	DataBitSet<PlantSpeciesId> m_annual;
	DataBitSet<PlantSpeciesId> m_growsInSunLight;
	DataBitSet<PlantSpeciesId> m_isTree;
	// Harvest
	DataVector<ItemTypeId, PlantSpeciesId> m_fruitItemType;
	DataVector<Step, PlantSpeciesId> m_stepsDurationHarvest;
	DataVector<Quantity, PlantSpeciesId> m_itemQuantityToHarvest;
	DataVector<uint16_t, PlantSpeciesId> m_dayOfYearToStartHarvest;
public:
	static void create(PlantSpeciesParamaters& paramaters);
	[[nodiscard]] static std::vector<ShapeId> getShapes(PlantSpeciesId species) { return data.m_shapes.at(species); };
	[[nodiscard]] static std::string getName(PlantSpeciesId species) { return data.m_name.at(species); };
	[[nodiscard]] static FluidTypeId getFluidType(PlantSpeciesId species) { return data.m_fluidType.at(species); };
	[[nodiscard]] static MaterialTypeId getWoodType(PlantSpeciesId species) { return data.m_woodType.at(species); };
	[[nodiscard]] static Step getStepsNeedsFluidFrequency(PlantSpeciesId species) { return data.m_stepsNeedsFluidFrequency.at(species); };
	[[nodiscard]] static Step getStepsTillDieWithoutFluid(PlantSpeciesId species) { return data.m_stepsTillDieWithoutFluid.at(species); };
	[[nodiscard]] static Step getStepsTillFullyGrown(PlantSpeciesId species) { return data.m_stepsTillFullyGrown.at(species); };
	[[nodiscard]] static Step getStepsTillFoliageGrowsFromZero(PlantSpeciesId species) { return data.m_stepsTillFoliageGrowsFromZero.at(species); };
	[[nodiscard]] static Step getStepsTillDieFromTemperature(PlantSpeciesId species) { return data.m_stepsTillDieFromTemperature.at(species); };
	[[nodiscard]] static DistanceInBlocks getRootRangeMax(PlantSpeciesId species) { return data.m_rootRangeMax.at(species); };
	[[nodiscard]] static DistanceInBlocks getRootRangeMin(PlantSpeciesId species) { return data.m_rootRangeMin.at(species); };
	[[nodiscard]] static Quantity getLogsGeneratedByFellingWhenFullGrown(PlantSpeciesId species) { return data.m_logsGeneratedByFellingWhenFullGrown.at(species); };
	[[nodiscard]] static Quantity getBranchesGeneratedByFellingWhenFullGrown(PlantSpeciesId species) { return data.m_branchesGeneratedByFellingWhenFullGrown.at(species); };
	[[nodiscard]] static Mass getAdultMass(PlantSpeciesId species) { return data.m_adultMass.at(species); };
	[[nodiscard]] static Temperature getMaximumGrowingTemperature(PlantSpeciesId species) { return data.m_maximumGrowingTemperature.at(species); };
	[[nodiscard]] static Temperature getMinimumGrowingTemperature(PlantSpeciesId species) { return data.m_minimumGrowingTemperature.at(species); };
	[[nodiscard]] static Volume getVolumeFluidConsumed(PlantSpeciesId species) { return data.m_volumeFluidConsumed.at(species); };
	[[nodiscard]] static uint16_t getDayOfYearForSowStart(PlantSpeciesId species) { return data.m_dayOfYearForSowStart.at(species); };
	[[nodiscard]] static uint16_t getDayOfYearForSowEnd(PlantSpeciesId species) { return data.m_dayOfYearForSowEnd.at(species); };
	[[nodiscard]] static uint8_t getMaxWildGrowth(PlantSpeciesId species) { return data.m_maxWildGrowth.at(species); };
	[[nodiscard]] static bool getAnnual(PlantSpeciesId species) { return data.m_annual.at(species); };
	[[nodiscard]] static bool getGrowsInSunLight(PlantSpeciesId species) { return data.m_growsInSunLight.at(species); };
	[[nodiscard]] static bool getIsTree(PlantSpeciesId species) { return data.m_isTree.at(species); };
	// returns base shape and wild growth steps.
	[[nodiscard]] static const std::pair<ShapeId, uint8_t> shapeAndWildGrowthForPercentGrown(PlantSpeciesId species, Percent percentGrown);
	[[nodiscard]] static ShapeId shapeForPercentGrown(PlantSpeciesId species, Percent percentGrown) { return shapeAndWildGrowthForPercentGrown(species, percentGrown).first; }
	[[nodiscard]] static uint8_t wildGrowthForPercentGrown(PlantSpeciesId species, Percent percentGrown) { return shapeAndWildGrowthForPercentGrown(species, percentGrown).second; }
	[[nodiscard]] static PlantSpeciesId byName(std::string name);
	// Harvest.
	ItemTypeId static getFruitItemType(PlantSpeciesId species) { return data.m_fruitItemType.at(species); }
	Step static getStepsDurationHarvest(PlantSpeciesId species) { return data.m_stepsDurationHarvest.at(species); }
	Quantity static getItemQuantityToHarvest(PlantSpeciesId species) { return data.m_itemQuantityToHarvest.at(species); }
	uint16_t static getDayOfYearToStartHarvest(PlantSpeciesId species) { return data.m_dayOfYearToStartHarvest.at(species); }
};
