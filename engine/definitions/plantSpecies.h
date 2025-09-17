#pragma once

#include "numericTypes/types.h"
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
	const Distance rootRangeMax = Distance::null();
	const Distance rootRangeMin = Distance::null();
	const Quantity logsGeneratedByFellingWhenFullGrown = Quantity::null();
	const Quantity branchesGeneratedByFellingWhenFullGrown = Quantity::null();
	const Mass adultMass = Mass::null();
	const Temperature maximumGrowingTemperature = Temperature::null();
	const Temperature minimumGrowingTemperature = Temperature::null();
	const FullDisplacement volumeFluidConsumed = FullDisplacement::null();
	const uint16_t dayOfYearForSowStart = 0;
	const uint16_t dayOfYearForSowEnd = 0;
	const uint8_t maxWildGrowth = 0;
	const bool annual = false;
	const bool growsInSunLight = false;
	const bool isTree = false;
	ItemTypeId fruitItemType = ItemTypeId::null();
	Step stepsDurationHarvest = Step::null();
	Quantity itemQuantityToHarvest = Quantity::null();
	uint16_t dayOfYearToStartHarvest = 0;
};
class PlantSpecies final
{
	StrongVector<std::vector<ShapeId>, PlantSpeciesId> m_shapes;
	StrongVector<std::string, PlantSpeciesId> m_name;
	StrongVector<FluidTypeId, PlantSpeciesId> m_fluidType;
	StrongVector<MaterialTypeId, PlantSpeciesId> m_woodType;
	StrongVector<Step, PlantSpeciesId> m_stepsNeedsFluidFrequency;
	StrongVector<Step, PlantSpeciesId> m_stepsTillDieWithoutFluid;
	StrongVector<Step, PlantSpeciesId> m_stepsTillFullyGrown;
	StrongVector<Step, PlantSpeciesId> m_stepsTillFoliageGrowsFromZero;
	StrongVector<Step, PlantSpeciesId> m_stepsTillDieFromTemperature;
	StrongVector<Distance, PlantSpeciesId> m_rootRangeMax;
	StrongVector<Distance, PlantSpeciesId> m_rootRangeMin;
	StrongVector<Quantity, PlantSpeciesId> m_logsGeneratedByFellingWhenFullGrown;
	StrongVector<Quantity, PlantSpeciesId> m_branchesGeneratedByFellingWhenFullGrown;
	StrongVector<Mass, PlantSpeciesId> m_adultMass;
	StrongVector<Temperature, PlantSpeciesId> m_maximumGrowingTemperature;
	StrongVector<Temperature, PlantSpeciesId> m_minimumGrowingTemperature;
	StrongVector<FullDisplacement, PlantSpeciesId> m_volumeFluidConsumed;
	StrongVector<uint16_t, PlantSpeciesId> m_dayOfYearForSowStart;
	StrongVector<uint16_t, PlantSpeciesId> m_dayOfYearForSowEnd;
	StrongVector<uint8_t, PlantSpeciesId> m_maxWildGrowth;
	StrongBitSet<PlantSpeciesId> m_annual;
	StrongBitSet<PlantSpeciesId> m_growsInSunLight;
	StrongBitSet<PlantSpeciesId> m_isTree;
	// Harvest
	StrongVector<ItemTypeId, PlantSpeciesId> m_fruitItemType;
	StrongVector<Step, PlantSpeciesId> m_stepsDurationHarvest;
	StrongVector<Quantity, PlantSpeciesId> m_itemQuantityToHarvest;
	StrongVector<uint16_t, PlantSpeciesId> m_dayOfYearToStartHarvest;
public:
	static void create(PlantSpeciesParamaters& paramaters);
	[[nodiscard]] static PlantSpeciesId size();
	[[nodiscard]] static std::vector<ShapeId> getShapes(const PlantSpeciesId& species);
	[[nodiscard]] static std::string getName(const PlantSpeciesId& species);
	[[nodiscard]] static FluidTypeId getFluidType(const PlantSpeciesId& species);
	[[nodiscard]] static MaterialTypeId getWoodType(const PlantSpeciesId& species);
	[[nodiscard]] static Step getStepsNeedsFluidFrequency(const PlantSpeciesId& species);
	[[nodiscard]] static Step getStepsTillDieWithoutFluid(const PlantSpeciesId& species);
	[[nodiscard]] static Step getStepsTillFullyGrown(const PlantSpeciesId& species);
	[[nodiscard]] static Step getStepsTillFoliageGrowsFromZero(const PlantSpeciesId& species);
	[[nodiscard]] static Step getStepsTillDieFromTemperature(const PlantSpeciesId& species);
	[[nodiscard]] static Distance getRootRangeMax(const PlantSpeciesId& species);
	[[nodiscard]] static Distance getRootRangeMin(const PlantSpeciesId& species);
	[[nodiscard]] static Quantity getLogsGeneratedByFellingWhenFullGrown(const PlantSpeciesId& species);
	[[nodiscard]] static Quantity getBranchesGeneratedByFellingWhenFullGrown(const PlantSpeciesId& species);
	[[nodiscard]] static Mass getAdultMass(const PlantSpeciesId& species);
	[[nodiscard]] static Temperature getMaximumGrowingTemperature(const PlantSpeciesId& species);
	[[nodiscard]] static Temperature getMinimumGrowingTemperature(const PlantSpeciesId& species);
	[[nodiscard]] static FullDisplacement getVolumeFluidConsumed(const PlantSpeciesId& species);
	[[nodiscard]] static uint16_t getDayOfYearForSowStart(const PlantSpeciesId& species);
	[[nodiscard]] static uint16_t getDayOfYearForSowEnd(const PlantSpeciesId& species);
	[[nodiscard]] static uint8_t getMaxWildGrowth(const PlantSpeciesId& species);
	[[nodiscard]] static bool getAnnual(const PlantSpeciesId& species);
	[[nodiscard]] static bool getGrowsInSunLight(const PlantSpeciesId& species);
	[[nodiscard]] static bool getIsTree(const PlantSpeciesId& species);
	// returns base shape and wild growth steps.
	[[nodiscard]] static const std::pair<ShapeId, uint8_t> shapeAndWildGrowthForPercentGrown(const PlantSpeciesId& species, const Percent& percentGrown);
	[[nodiscard]] static ShapeId shapeForPercentGrown(const PlantSpeciesId& species, const Percent& percentGrown);
	[[nodiscard]] static uint8_t wildGrowthForPercentGrown(const PlantSpeciesId& species, const Percent& percentGrown);
	[[nodiscard]] static PlantSpeciesId byName(std::string name);
	// Harvest.
	ItemTypeId static getFruitItemType(const PlantSpeciesId& species);
	Step static getStepsDurationHarvest(const PlantSpeciesId& species);
	Quantity static getItemQuantityToHarvest(const PlantSpeciesId& species);
	uint16_t static getDayOfYearToStartHarvest(const PlantSpeciesId& species);
};
inline PlantSpecies g_plantSpeciesData;
