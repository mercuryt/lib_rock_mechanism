#include "fluidType.h"
const FluidType& FluidType::byName(const std::string name)
{
	auto found = std::ranges::find(fluidTypeDataStore, name, &FluidType::name);
	assert(found != fluidTypeDataStore.end());
	return *found;
}
FluidType& FluidType::byNameNonConst(const std::string name)
{
	auto found = std::ranges::find(fluidTypeDataStore, name, &FluidType::name);
	assert(found != fluidTypeDataStore.end());
	return *found;
}
