#pragma once

#include "types.h"

#include <string>
#include <cassert>
#include <vector>
#include <algorithm>
#include <ranges>

struct MaterialType;

struct FluidType
{
	const std::string name;
	const uint32_t viscosity;
	const uint32_t density;
	const Step mistDuration;
	const uint32_t maxMistSpread;
	const MaterialType* freezesInto;
	// Infastructure.
	bool operator==(const FluidType& fluidType) const { return this == &fluidType; }
	inline static std::vector<FluidType> data;
	static const FluidType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &FluidType::name);
		assert(found != data.end());
		return *found;
	}
	static FluidType& byNameNonConst(const std::string name)
	{
		auto found = std::ranges::find(data, name, &FluidType::name);
		assert(found != data.end());
		return *found;
	}
};
