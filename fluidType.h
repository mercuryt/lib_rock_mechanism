#pragma once

struct FluidType
{
	std::string name;
	uint32_t viscosity;
	uint32_t density;
	FluidType(std::string& n, uint32_t v, uint32_t d) : name(n), viscosity(v), density(d) {}
};

static std::vector<FluidType> fluidTypes;

FluidType* registerFluidType(std::string name, uint32_t viscosity, uint32_t density)
{
	fluidTypes.emplace_back(name, viscosity, density);
	return &fluidTypes.back();

}
