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
	static FluidType data;
	DataVector<std::string, FluidTypeId> m_name;
	DataVector<uint32_t, FluidTypeId> m_viscosity;
	DataVector<Density, FluidTypeId> m_density;
	DataVector<Step, FluidTypeId> m_mistDuration;
	DataVector<DistanceInBlocks, FluidTypeId> m_maxMistSpread;
	DataVector<MaterialTypeId, FluidTypeId> m_freezesInto;
public:
	[[nodiscard]] static const FluidType& byName(const std::string name);
	[[nodiscard]] static FluidType& byNameNonConst(const std::string name);
	static FluidTypeId create(FluidTypeParamaters& p)
	{
		data.m_name.add(p.name);
		data.m_viscosity.add(p.viscosity);
		data.m_density.add(p.density);
		data.m_mistDuration.add(p.mistDuration);
		data.m_maxMistSpread.add(p.maxMistSpread);
		data.m_freezesInto.add(p.freezesInto);
		return FluidTypeId::create(data.m_name.size() - 1);
	}
	[[nodiscard]] static std::string getname(FluidTypeId id) { return data.m_name.at(id); };
	[[nodiscard]] static uint32_t getviscosity(FluidTypeId id) { return data.m_viscosity.at(id); };
	[[nodiscard]] static Density getdensity(FluidTypeId id) { return data.m_density.at(id); };
	[[nodiscard]] static Step getmistDuration(FluidTypeId id) { return data.m_mistDuration.at(id); };
	[[nodiscard]] static DistanceInBlocks getmaxMistSpread(FluidTypeId id) { return data.m_maxMistSpread.at(id); };
	[[nodiscard]] static MaterialTypeId getfreezesInto(FluidTypeId id) { return data.m_freezesInto.at(id); };
};
