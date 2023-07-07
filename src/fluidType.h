#pragma once
#include <string>
#include <cassert>

struct FluidType
{
	const std::string name;
	const uint32_t viscosity;
	const uint32_t density;
	const uint32_t mistDuration;
	const uint32_t maxMistSpread;
	// Infastructure.
	bool operator==(const FluidType& fluidType){ return this == &fluidType; }
	static std::vector<FluidType> data;
	static const FluidType& byName(std::string name)
	{
		auto found = std::ranges::find(data, name, &FluidType::name);
		assert(found != data.end());
		return *found;
	}
};
