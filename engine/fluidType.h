#pragma once

#include "config.h"
#include "dataVector.h"
#include "types.h"

#include <string>
#include <cassert>
#include <vector>
#include <algorithm>
#include <ranges>

struct FluidTypeParamaters
{
	std::string name;
	uint32_t viscosity;
	Density density;
	Step mistDuration;
	DistanceInBlocks maxMistSpread;
	MaterialTypeId freezesInto = MaterialTypeId::null();
};

class FluidType
{
	DataVector<std::string, FluidTypeId> m_name;
	DataVector<uint32_t, FluidTypeId> m_viscosity;
	DataVector<Density, FluidTypeId> m_density;
	DataVector<Step, FluidTypeId> m_mistDuration;
	DataVector<DistanceInBlocks, FluidTypeId> m_maxMistSpread;
	DataVector<MaterialTypeId, FluidTypeId> m_freezesInto;
public:
	[[nodiscard]] static const FluidTypeId byName(const std::string name);
	static FluidTypeId create(FluidTypeParamaters& p);
	static void setFreezesInto(FluidTypeId fluid, MaterialTypeId material);
	[[nodiscard]] static std::string getName(FluidTypeId id);
	[[nodiscard]] static uint32_t getViscosity(FluidTypeId id);
	[[nodiscard]] static Density getDensity(FluidTypeId id);
	[[nodiscard]] static Step getMistDuration(FluidTypeId id);
	[[nodiscard]] static DistanceInBlocks getMaxMistSpread(FluidTypeId id);
	[[nodiscard]] static MaterialTypeId getFreezesInto(FluidTypeId id);
};
inline FluidType fluidTypeData;
