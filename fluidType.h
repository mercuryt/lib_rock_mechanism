#pragma once
#include <list>
#include <string>
#include <algorithm>
#include <cassert>
#include <ranges>
#include <iterator>

struct FluidType
{
	const std::string name;
	const uint32_t viscosity;
	const uint32_t density;
	const uint32_t mistDuration;
	const uint32_t maxMistSpread;
	FluidType(std::string n, uint32_t v, uint32_t d, uint32_t md, uint32_t mms) :
	       	name(n), viscosity(v), density(d), mistDuration(md), maxMistSpread(mms) {}
};

static std::list<FluidType> fluidTypes;

const FluidType* registerFluidType(std::string name, uint32_t viscosity, uint32_t density, uint32_t mistDuration, uint32_t maxMistSpread)
{
	// Density must be unique because it is used as the key in block::m_fluids.
	assert(std::ranges::find_if(fluidTypes, [&](const FluidType& fluidType){ return fluidType.density == density; }) == fluidTypes.end());
	fluidTypes.emplace_back(name, viscosity, density, mistDuration, maxMistSpread);
	return &fluidTypes.back();

}
