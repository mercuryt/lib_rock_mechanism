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
	std::wstring name;
	uint32_t viscosity;
	Density density;
	Step mistDuration;
	DistanceInBlocks maxMistSpread;
	MaterialTypeId freezesInto = MaterialTypeId::null();
	float evaporationRate = 0.f;
};

class FluidType
{
	DataVector<std::wstring, FluidTypeId> m_name;
	DataVector<uint32_t, FluidTypeId> m_viscosity;
	DataVector<Density, FluidTypeId> m_density;
	DataVector<Step, FluidTypeId> m_mistDuration;
	DataVector<DistanceInBlocks, FluidTypeId> m_maxMistSpread;
	DataVector<MaterialTypeId, FluidTypeId> m_freezesInto;
	DataVector<float, FluidTypeId> m_evaporationRate;
public:
	[[nodiscard]] static const FluidTypeId byName(const std::wstring name);
	[[nodiscard]] static FluidTypeId size();
	static FluidTypeId create(FluidTypeParamaters& p);
	static void setFreezesInto(const FluidTypeId& fluid, const MaterialTypeId& material);
	[[nodiscard]] static std::wstring getName(const FluidTypeId& id);
	[[nodiscard]] static uint32_t getViscosity(const FluidTypeId& id);
	[[nodiscard]] static Density getDensity(const FluidTypeId& id);
	[[nodiscard]] static Step getMistDuration(const FluidTypeId& id);
	[[nodiscard]] static DistanceInBlocks getMaxMistSpread(const FluidTypeId& id);
	[[nodiscard]] static MaterialTypeId getFreezesInto(const FluidTypeId& id);
	[[nodiscard]] static float getEvaporationRate(const FluidTypeId& id);
};
inline FluidType fluidTypeData;
