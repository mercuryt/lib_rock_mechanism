#pragma once

#include "config.h"
#include "dataStructures/strongVector.h"
#include "numericTypes/types.h"

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
	Distance maxMistSpread;
	MaterialTypeId freezesInto = MaterialTypeId::null();
	float evaporationRate = 0.f;
};

class FluidType
{
	StrongVector<std::string, FluidTypeId> m_name;
	StrongVector<uint32_t, FluidTypeId> m_viscosity;
	StrongVector<Density, FluidTypeId> m_density;
	StrongVector<Step, FluidTypeId> m_mistDuration;
	StrongVector<Distance, FluidTypeId> m_maxMistSpread;
	StrongVector<MaterialTypeId, FluidTypeId> m_freezesInto;
	StrongVector<float, FluidTypeId> m_evaporationRate;
public:
	[[nodiscard]] static const FluidTypeId byName(const std::string name);
	[[nodiscard]] static FluidTypeId size();
	static FluidTypeId create(FluidTypeParamaters& p);
	static void setFreezesInto(const FluidTypeId& fluid, const MaterialTypeId& material);
	[[nodiscard]] static std::string getName(const FluidTypeId& id);
	[[nodiscard]] static std::string maybeGetName(const FluidTypeId& id);
	[[nodiscard]] static uint32_t getViscosity(const FluidTypeId& id);
	[[nodiscard]] static Density getDensity(const FluidTypeId& id);
	[[nodiscard]] static Step getMistDuration(const FluidTypeId& id);
	[[nodiscard]] static Distance getMaxMistSpread(const FluidTypeId& id);
	[[nodiscard]] static MaterialTypeId getFreezesInto(const FluidTypeId& id);
	[[nodiscard]] static float getEvaporationRate(const FluidTypeId& id);
	[[nodiscard]] static Temperature getFreezingPoint(const FluidTypeId& id);
};
inline FluidType fluidTypeData;
