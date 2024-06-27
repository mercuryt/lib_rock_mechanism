#include "plantSpecies.h"

#include <algorithm>
#include <ranges>
const std::pair<const Shape*, uint8_t> PlantSpecies::shapeAndWildGrowthForPercentGrown(Percent percentGrown) const
{
	uint8_t totalGrowthStages = shapes.size() + maxWildGrowth;
	uint8_t growthStage = util::scaleByPercentRange(1, totalGrowthStages, percentGrown);
	size_t index = std::min(static_cast<size_t>(growthStage), shapes.size()) - 1;
	uint8_t wildGrowthSteps = shapes.size() < growthStage ? growthStage - shapes.size() : 0u;
	return std::make_pair(shapes.at(index), wildGrowthSteps);
}
// Static method.
const PlantSpecies& PlantSpecies::byName(std::string name)
{
	auto found = std::ranges::find(plantSpeciesDataStore, name, &PlantSpecies::name);
	assert(found != plantSpeciesDataStore.end());
	return *found;
}
