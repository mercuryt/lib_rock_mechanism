#include "definitions/plantSpecies.h"

#include <algorithm>
#include <ranges>

void PlantSpecies::create(PlantSpeciesParamaters& p)
{
	g_plantSpeciesData.m_shapes.add(p.shapes);
	g_plantSpeciesData.m_name.add(p.name);
	g_plantSpeciesData.m_fluidType.add(p.fluidType);
	g_plantSpeciesData.m_woodType.add(p.woodType);
	g_plantSpeciesData.m_stepsNeedsFluidFrequency.add(p.stepsNeedsFluidFrequency);
	g_plantSpeciesData.m_stepsTillDieWithoutFluid.add(p.stepsTillDieWithoutFluid);
	g_plantSpeciesData.m_stepsTillFullyGrown.add(p.stepsTillFullyGrown);
	g_plantSpeciesData.m_stepsTillFoliageGrowsFromZero.add(p.stepsTillFoliageGrowsFromZero);
	g_plantSpeciesData.m_stepsTillDieFromTemperature.add(p.stepsTillDieFromTemperature);
	g_plantSpeciesData.m_rootRangeMax.add(p.rootRangeMax);
	g_plantSpeciesData.m_rootRangeMin.add(p.rootRangeMin);
	g_plantSpeciesData.m_logsGeneratedByFellingWhenFullGrown.add(p.logsGeneratedByFellingWhenFullGrown);
	g_plantSpeciesData.m_branchesGeneratedByFellingWhenFullGrown.add(p.branchesGeneratedByFellingWhenFullGrown);
	g_plantSpeciesData.m_adultMass.add(p.adultMass);
	g_plantSpeciesData.m_maximumGrowingTemperature.add(p.maximumGrowingTemperature);
	g_plantSpeciesData.m_minimumGrowingTemperature.add(p.minimumGrowingTemperature);
	g_plantSpeciesData.m_volumeFluidConsumed.add(p.volumeFluidConsumed);
	g_plantSpeciesData.m_dayOfYearForSowStart.add(p.dayOfYearForSowStart);
	g_plantSpeciesData.m_dayOfYearForSowEnd.add(p.dayOfYearForSowEnd);
	g_plantSpeciesData.m_maxWildGrowth.add(p.maxWildGrowth);
	g_plantSpeciesData.m_annual.add(p.annual);
	g_plantSpeciesData.m_growsInSunLight.add(p.growsInSunLight);
	g_plantSpeciesData.m_isTree.add(p.isTree);
	g_plantSpeciesData.m_fruitItemType.add(p.fruitItemType);
	g_plantSpeciesData.m_stepsDurationHarvest.add(p.stepsDurationHarvest);
	g_plantSpeciesData.m_itemQuantityToHarvest.add(p.itemQuantityToHarvest);
	g_plantSpeciesData.m_dayOfYearToStartHarvest.add(p.dayOfYearToStartHarvest);
}
const std::pair<ShapeId, int> PlantSpecies::shapeAndWildGrowthForPercentGrown(const PlantSpeciesId& species, const Percent& percentGrown)
{
	size_t totalGrowthStages = g_plantSpeciesData.m_shapes[species].size() + (size_t)g_plantSpeciesData.m_maxWildGrowth[species];
	size_t growthStage = util::scaleByPercentRange(1, totalGrowthStages, percentGrown);
	size_t index = std::min(static_cast<size_t>(growthStage), g_plantSpeciesData.m_shapes[species].size()) - 1;
	int wildGrowthSteps = g_plantSpeciesData.m_shapes[species].size() < growthStage ? growthStage - g_plantSpeciesData.m_shapes[species].size() : 0u;
	return std::make_pair(g_plantSpeciesData.m_shapes[species][index], wildGrowthSteps);
}
PlantSpeciesId PlantSpecies::size() {return PlantSpeciesId::create(g_plantSpeciesData.m_name.size()); }
std::vector<ShapeId> PlantSpecies::getShapes(const PlantSpeciesId& species) { return g_plantSpeciesData.m_shapes[species]; };
std::string PlantSpecies::getName(const PlantSpeciesId& species) { return g_plantSpeciesData.m_name[species]; };
FluidTypeId PlantSpecies::getFluidType(const PlantSpeciesId& species) { return g_plantSpeciesData.m_fluidType[species]; };
MaterialTypeId PlantSpecies::getWoodType(const PlantSpeciesId& species) { return g_plantSpeciesData.m_woodType[species]; };
Step PlantSpecies::getStepsNeedsFluidFrequency(const PlantSpeciesId& species) { return g_plantSpeciesData.m_stepsNeedsFluidFrequency[species]; };
Step PlantSpecies::getStepsTillDieWithoutFluid(const PlantSpeciesId& species) { return g_plantSpeciesData.m_stepsTillDieWithoutFluid[species]; };
Step PlantSpecies::getStepsTillFullyGrown(const PlantSpeciesId& species) { return g_plantSpeciesData.m_stepsTillFullyGrown[species]; };
Step PlantSpecies::getStepsTillFoliageGrowsFromZero(const PlantSpeciesId& species) { return g_plantSpeciesData.m_stepsTillFoliageGrowsFromZero[species]; };
Step PlantSpecies::getStepsTillDieFromTemperature(const PlantSpeciesId& species) { return g_plantSpeciesData.m_stepsTillDieFromTemperature[species]; };
Distance PlantSpecies::getRootRangeMax(const PlantSpeciesId& species) { return g_plantSpeciesData.m_rootRangeMax[species]; };
Distance PlantSpecies::getRootRangeMin(const PlantSpeciesId& species) { return g_plantSpeciesData.m_rootRangeMin[species]; };
Quantity PlantSpecies::getLogsGeneratedByFellingWhenFullGrown(const PlantSpeciesId& species) { return g_plantSpeciesData.m_logsGeneratedByFellingWhenFullGrown[species]; };
Quantity PlantSpecies::getBranchesGeneratedByFellingWhenFullGrown(const PlantSpeciesId& species) { return g_plantSpeciesData.m_branchesGeneratedByFellingWhenFullGrown[species]; };
Mass PlantSpecies::getAdultMass(const PlantSpeciesId& species) { return g_plantSpeciesData.m_adultMass[species]; };
Temperature PlantSpecies::getMaximumGrowingTemperature(const PlantSpeciesId& species) { return g_plantSpeciesData.m_maximumGrowingTemperature[species]; };
Temperature PlantSpecies::getMinimumGrowingTemperature(const PlantSpeciesId& species) { return g_plantSpeciesData.m_minimumGrowingTemperature[species]; };
FullDisplacement PlantSpecies::getVolumeFluidConsumed(const PlantSpeciesId& species) { return g_plantSpeciesData.m_volumeFluidConsumed[species]; };
int PlantSpecies::getDayOfYearForSowStart(const PlantSpeciesId& species) { return g_plantSpeciesData.m_dayOfYearForSowStart[species]; };
int PlantSpecies::getDayOfYearForSowEnd(const PlantSpeciesId& species) { return g_plantSpeciesData.m_dayOfYearForSowEnd[species]; };
int PlantSpecies::getMaxWildGrowth(const PlantSpeciesId& species) { return g_plantSpeciesData.m_maxWildGrowth[species]; };
bool PlantSpecies::getAnnual(const PlantSpeciesId& species) { return g_plantSpeciesData.m_annual[species]; };
bool PlantSpecies::getGrowsInSunLight(const PlantSpeciesId& species) { return g_plantSpeciesData.m_growsInSunLight[species]; };
bool PlantSpecies::getIsTree(const PlantSpeciesId& species) { return g_plantSpeciesData.m_isTree[species]; };
// returns base shape and wild growth steps.
ShapeId PlantSpecies::shapeForPercentGrown(const PlantSpeciesId& species, const Percent& percentGrown) { return shapeAndWildGrowthForPercentGrown(species, percentGrown).first; }
int PlantSpecies::wildGrowthForPercentGrown(const PlantSpeciesId& species, const Percent& percentGrown) { return shapeAndWildGrowthForPercentGrown(species, percentGrown).second; }
// Static method.
PlantSpeciesId PlantSpecies::byName(std::string name)
{
	auto found = g_plantSpeciesData.m_name.find(name);
	assert(found != g_plantSpeciesData.m_name.end());
	return PlantSpeciesId::create(found - g_plantSpeciesData.m_name.begin());
}
// Harvest.
ItemTypeId PlantSpecies::getFruitItemType(const PlantSpeciesId& species) { return g_plantSpeciesData.m_fruitItemType[species]; }
Step PlantSpecies::getStepsDurationHarvest(const PlantSpeciesId& species) { return g_plantSpeciesData.m_stepsDurationHarvest[species]; }
Quantity PlantSpecies::getItemQuantityToHarvest(const PlantSpeciesId& species) { return g_plantSpeciesData.m_itemQuantityToHarvest[species]; }
int PlantSpecies::getDayOfYearToStartHarvest(const PlantSpeciesId& species) { return g_plantSpeciesData.m_dayOfYearToStartHarvest[species]; }
