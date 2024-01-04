#pragma once

#include "config.h"
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
	const Density density;
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
inline void to_json(Json& data, const FluidType* const& fluidType){ data = fluidType->name; }
inline void to_json(Json& data, const FluidType& fluidType){ data = fluidType.name; }
inline void from_json(const Json& data, const FluidType*& fluidType){ fluidType = &FluidType::byName(data["fluidType"].get<std::string>()); }
