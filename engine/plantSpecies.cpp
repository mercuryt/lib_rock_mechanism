#include "plantSpecies.h"

#include <algorithm>
#include <ranges>

void PlantSpecies::create(PlantSpeciesParamaters& p)
{
	data.m_shapes.add(p.shapes);
	data.m_name.add(p.name);
	data.m_fluidType.add(p.fluidType);
	data.m_woodType.add(p.woodType);
	data.m_stepsNeedsFluidFrequency.add(p.stepsNeedsFluidFrequency);
	data.m_stepsTillDieWithoutFluid.add(p.stepsTillDieWithoutFluid);
	data.m_stepsTillFullyGrown.add(p.stepsTillFullyGrown);
	data.m_stepsTillFoliageGrowsFromZero.add(p.stepsTillFoliageGrowsFromZero);
	data.m_stepsTillDieFromTemperature.add(p.stepsTillDieFromTemperature);
	data.m_rootRangeMax.add(p.rootRangeMax);
	data.m_rootRangeMin.add(p.rootRangeMin);
	data.m_logsGeneratedByFellingWhenFullGrown.add(p.logsGeneratedByFellingWhenFullGrown);
	data.m_branchesGeneratedByFellingWhenFullGrown.add(p.branchesGeneratedByFellingWhenFullGrown);
	data.m_adultMass.add(p.adultMass);
	data.m_maximumGrowingTemperature.add(p.maximumGrowingTemperature);
	data.m_minimumGrowingTemperature.add(p.minimumGrowingTemperature);
	data.m_volumeFluidConsumed.add(p.volumeFluidConsumed);
	data.m_dayOfYearForSowStart.add(p.dayOfYearForSowStart);
	data.m_dayOfYearForSowEnd.add(p.dayOfYearForSowEnd);
	data.m_maxWildGrowth.add(p.maxWildGrowth);
	data.m_annual.add(p.annual);
	data.m_growsInSunLight.add(p.growsInSunLight);
	data.m_isTree.add(p.isTree);
	data.m_fruitItemType.add(p.fruitItemType);
	data.m_stepsDurationHarvest.add(p.stepsDurationHarvest);
	data.m_itemQuantityToHarvest.add(p.itemQuantityToHarvest);
	data.m_dayOfYearToStartHarvest.add(p.dayOfYearToStartHarvest);
}
const std::pair<ShapeId, uint8_t> PlantSpecies::shapeAndWildGrowthForPercentGrown(PlantSpeciesId species, Percent percentGrown)
{
	uint8_t totalGrowthStages = data.m_shapes.at(species).size() + data.m_maxWildGrowth.at(species);
	uint8_t growthStage = util::scaleByPercentRange(1, totalGrowthStages, percentGrown);
	size_t index = std::min(static_cast<size_t>(growthStage), data.m_shapes.at(species).size()) - 1;
	uint8_t wildGrowthSteps = data.m_shapes.at(species).size() < growthStage ? growthStage - data.m_shapes.at(species).size() : 0u;
	return std::make_pair(data.m_shapes.at(species).at(index), wildGrowthSteps);
}
// Static method.
PlantSpeciesId PlantSpecies::byName(std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return PlantSpeciesId::create(found - data.m_name.begin());
}
