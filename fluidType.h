#pragma once
#include <list>
#include <string>

struct FluidType
{
	const std::string name;
	const uint32_t viscosity;
	const uint32_t density;
	FluidType(std::string n, uint32_t v, uint32_t d) : name(n), viscosity(v), density(d) {}
};

static std::list<FluidType> fluidTypes;

const FluidType* registerFluidType(std::string name, uint32_t viscosity, uint32_t density)
{
	fluidTypes.emplace_back(name, viscosity, density);
	return &fluidTypes.back();

}
