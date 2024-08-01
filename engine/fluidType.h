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
	const MaterialType* freezesInto = nullptr;
	const Step mistDuration = Step::create(0);
	const uint32_t viscosity = 0;
	const DistanceInBlocks maxMistSpread = DistanceInBlocks::create(0);
	const Density density = Density::create(0);
	// Infastructure.
	[[nodiscard]] bool operator==(const FluidType& fluidType) const { return this == &fluidType; }
	[[nodiscard]] static const FluidType& byName(const std::string name);
	[[nodiscard]] static FluidType& byNameNonConst(const std::string name);
	FluidType(const std::string n, const uint32_t v, const Density d, const Step md, const DistanceInBlocks mms):
	name(n), mistDuration(md), viscosity(v), maxMistSpread(mms), density(d) { }
};
inline std::vector<FluidType> fluidTypeDataStore;
inline void to_json(Json& data, const FluidType* const& fluidType)
{
       	data = fluidType == nullptr ? "0" : fluidType->name;
}
inline void to_json(Json& data, const FluidType*& fluidType)
{
       	data = fluidType == nullptr ? "0" : fluidType->name;
}
inline void to_json(Json& data, const FluidType& fluidType){ data = fluidType.name; }
inline void from_json(const Json& data, const FluidType*& fluidType)
{ 
	std::string name = data.get<std::string>();
	fluidType = name == "0" ? nullptr : &FluidType::byName(name);
}
