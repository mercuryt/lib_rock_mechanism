#include "plantSpecies.h"

#include <algorithm>
#include <ranges>

void PlantSpecies::create(PlantSpeciesParamaters& p)
{
	plantSpeciesData.m_shapes.add(p.shapes);
	plantSpeciesData.m_name.add(p.name);
	plantSpeciesData.m_fluidType.add(p.fluidType);
	plantSpeciesData.m_woodType.add(p.woodType);
	plantSpeciesData.m_stepsNeedsFluidFrequency.add(p.stepsNeedsFluidFrequency);
	plantSpeciesData.m_stepsTillDieWithoutFluid.add(p.stepsTillDieWithoutFluid);
	plantSpeciesData.m_stepsTillFullyGrown.add(p.stepsTillFullyGrown);
	plantSpeciesData.m_stepsTillFoliageGrowsFromZero.add(p.stepsTillFoliageGrowsFromZero);
	plantSpeciesData.m_stepsTillDieFromTemperature.add(p.stepsTillDieFromTemperature);
	plantSpeciesData.m_rootRangeMax.add(p.rootRangeMax);
	plantSpeciesData.m_rootRangeMin.add(p.rootRangeMin);
	plantSpeciesData.m_logsGeneratedByFellingWhenFullGrown.add(p.logsGeneratedByFellingWhenFullGrown);
	plantSpeciesData.m_branchesGeneratedByFellingWhenFullGrown.add(p.branchesGeneratedByFellingWhenFullGrown);
	plantSpeciesData.m_adultMass.add(p.adultMass);
	plantSpeciesData.m_maximumGrowingTemperature.add(p.maximumGrowingTemperature);
	plantSpeciesData.m_minimumGrowingTemperature.add(p.minimumGrowingTemperature);
	plantSpeciesData.m_volumeFluidConsumed.add(p.volumeFluidConsumed);
	plantSpeciesData.m_dayOfYearForSowStart.add(p.dayOfYearForSowStart);
	plantSpeciesData.m_dayOfYearForSowEnd.add(p.dayOfYearForSowEnd);
	plantSpeciesData.m_maxWildGrowth.add(p.maxWildGrowth);
	plantSpeciesData.m_annual.add(p.annual);
	plantSpeciesData.m_growsInSunLight.add(p.growsInSunLight);
	plantSpeciesData.m_isTree.add(p.isTree);
	plantSpeciesData.m_fruitItemType.add(p.fruitItemType);
	plantSpeciesData.m_stepsDurationHarvest.add(p.stepsDurationHarvest);
	plantSpeciesData.m_itemQuantityToHarvest.add(p.itemQuantityToHarvest);
	plantSpeciesData.m_dayOfYearToStartHarvest.add(p.dayOfYearToStartHarvest);
}
const std::pair<ShapeId, uint8_t> PlantSpecies::shapeAndWildGrowthForPercentGrown(PlantSpeciesId species, Percent percentGrown)
{
	uint8_t totalGrowthStages = plantSpeciesData.m_shapes[species].size() + plantSpeciesData.m_maxWildGrowth[species];
	uint8_t growthStage = util::scaleByPercentRange(1, totalGrowthStages, percentGrown);
	size_t index = std::min(static_cast<size_t>(growthStage), plantSpeciesData.m_shapes[species].size()) - 1;
	uint8_t wildGrowthSteps = plantSpeciesData.m_shapes[species].size() < growthStage ? growthStage - plantSpeciesData.m_shapes[species].size() : 0u;
	return std::make_pair(plantSpeciesData.m_shapes[species][index], wildGrowthSteps);
}
std::vector<ShapeId> PlantSpecies::getShapes(PlantSpeciesId species) { return plantSpeciesData.m_shapes[species]; };
std::string PlantSpecies::getName(PlantSpeciesId species) { return plantSpeciesData.m_name[species]; };
FluidTypeId PlantSpecies::getFluidType(PlantSpeciesId species) { return plantSpeciesData.m_fluidType[species]; };
MaterialTypeId PlantSpecies::getWoodType(PlantSpeciesId species) { return plantSpeciesData.m_woodType[species]; };
Step PlantSpecies::getStepsNeedsFluidFrequency(PlantSpeciesId species) { return plantSpeciesData.m_stepsNeedsFluidFrequency[species]; };
Step PlantSpecies::getStepsTillDieWithoutFluid(PlantSpeciesId species) { return plantSpeciesData.m_stepsTillDieWithoutFluid[species]; };
Step PlantSpecies::getStepsTillFullyGrown(PlantSpeciesId species) { return plantSpeciesData.m_stepsTillFullyGrown[species]; };
Step PlantSpecies::getStepsTillFoliageGrowsFromZero(PlantSpeciesId species) { return plantSpeciesData.m_stepsTillFoliageGrowsFromZero[species]; };
Step PlantSpecies::getStepsTillDieFromTemperature(PlantSpeciesId species) { return plantSpeciesData.m_stepsTillDieFromTemperature[species]; };
DistanceInBlocks PlantSpecies::getRootRangeMax(PlantSpeciesId species) { return plantSpeciesData.m_rootRangeMax[species]; };
DistanceInBlocks PlantSpecies::getRootRangeMin(PlantSpeciesId species) { return plantSpeciesData.m_rootRangeMin[species]; };
Quantity PlantSpecies::getLogsGeneratedByFellingWhenFullGrown(PlantSpeciesId species) { return plantSpeciesData.m_logsGeneratedByFellingWhenFullGrown[species]; };
Quantity PlantSpecies::getBranchesGeneratedByFellingWhenFullGrown(PlantSpeciesId species) { return plantSpeciesData.m_branchesGeneratedByFellingWhenFullGrown[species]; };
Mass PlantSpecies::getAdultMass(PlantSpeciesId species) { return plantSpeciesData.m_adultMass[species]; };
Temperature PlantSpecies::getMaximumGrowingTemperature(PlantSpeciesId species) { return plantSpeciesData.m_maximumGrowingTemperature[species]; };
Temperature PlantSpecies::getMinimumGrowingTemperature(PlantSpeciesId species) { return plantSpeciesData.m_minimumGrowingTemperature[species]; };
Volume PlantSpecies::getVolumeFluidConsumed(PlantSpeciesId species) { return plantSpeciesData.m_volumeFluidConsumed[species]; };
uint16_t PlantSpecies::getDayOfYearForSowStart(PlantSpeciesId species) { return plantSpeciesData.m_dayOfYearForSowStart[species]; };
uint16_t PlantSpecies::getDayOfYearForSowEnd(PlantSpeciesId species) { return plantSpeciesData.m_dayOfYearForSowEnd[species]; };
uint8_t PlantSpecies::getMaxWildGrowth(PlantSpeciesId species) { return plantSpeciesData.m_maxWildGrowth[species]; };
bool PlantSpecies::getAnnual(PlantSpeciesId species) { return plantSpeciesData.m_annual[species]; };
bool PlantSpecies::getGrowsInSunLight(PlantSpeciesId species) { return plantSpeciesData.m_growsInSunLight[species]; };
bool PlantSpecies::getIsTree(PlantSpeciesId species) { return plantSpeciesData.m_isTree[species]; };
// returns base shape and wild growth steps.
ShapeId PlantSpecies::shapeForPercentGrown(PlantSpeciesId species, Percent percentGrown) { return shapeAndWildGrowthForPercentGrown(species, percentGrown).first; }
uint8_t PlantSpecies::wildGrowthForPercentGrown(PlantSpeciesId species, Percent percentGrown) { return shapeAndWildGrowthForPercentGrown(species, percentGrown).second; }
// Static method.
PlantSpeciesId PlantSpecies::byName(std::string name)
{
	auto found = plantSpeciesData.m_name.find(name);
	assert(found != plantSpeciesData.m_name.end());
	return PlantSpeciesId::create(found - plantSpeciesData.m_name.begin());
}
// Harvest.
ItemTypeId PlantSpecies::getFruitItemType(PlantSpeciesId species) { return plantSpeciesData.m_fruitItemType[species]; }
Step PlantSpecies::getStepsDurationHarvest(PlantSpeciesId species) { return plantSpeciesData.m_stepsDurationHarvest[species]; }
Quantity PlantSpecies::getItemQuantityToHarvest(PlantSpeciesId species) { return plantSpeciesData.m_itemQuantityToHarvest[species]; }
uint16_t PlantSpecies::getDayOfYearToStartHarvest(PlantSpeciesId species) { return plantSpeciesData.m_dayOfYearToStartHarvest[species]; }
