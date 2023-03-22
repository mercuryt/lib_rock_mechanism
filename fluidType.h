#pragma once
#include <list>
#include <string>

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
	fluidTypes.emplace_back(name, viscosity, density, mistDuration, maxMistSpread);
	return &fluidTypes.back();

}
