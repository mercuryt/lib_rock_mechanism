#include "animalSpecies.h"
const Shape& AnimalSpecies::shapeForPercentGrown(Percent percentGrown) const
{
	size_t index = util::scaleByPercentRange(0, shapes.size() - 1, percentGrown);
	return *shapes.at(index);
}
// Static method.
const AnimalSpecies& AnimalSpecies::byName(std::string name)
{
	auto found = std::ranges::find(animalSpeciesDataStore, name, &AnimalSpecies::name);
	assert(found != animalSpeciesDataStore.end());
	return *found;
}
