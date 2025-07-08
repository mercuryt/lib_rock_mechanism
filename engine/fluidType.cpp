#include "fluidType.h"
#include "definitions/materialType.h"
const FluidTypeId FluidType::byName(const std::string name)
{
	if(name == "none")
		return FluidTypeId::null();
	auto found = fluidTypeData.m_name.find(name);
	assert(found != fluidTypeData.m_name.end());
	return FluidTypeId::create(found - fluidTypeData.m_name.begin());
}
FluidTypeId FluidType::size() { return FluidTypeId::create(fluidTypeData.m_name.size()); }
FluidTypeId FluidType::create(FluidTypeParamaters& p)
{
	fluidTypeData.m_name.add(p.name);
	fluidTypeData.m_viscosity.add(p.viscosity);
	fluidTypeData.m_density.add(p.density);
	fluidTypeData.m_mistDuration.add(p.mistDuration);
	fluidTypeData.m_maxMistSpread.add(p.maxMistSpread);
	fluidTypeData.m_freezesInto.add(p.freezesInto);
	fluidTypeData.m_evaporationRate.add(p.evaporationRate);
	return FluidTypeId::create(fluidTypeData.m_name.size() - 1);
}
void FluidType::setFreezesInto(const FluidTypeId& fluid, const MaterialTypeId& material) { assert(fluid.exists()); assert(material.exists()); fluidTypeData.m_freezesInto[fluid] = material; }
std::string FluidType::getName(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_name[id]; };
std::string FluidType::maybeGetName(const FluidTypeId& id) { if(!id.exists()) return "none"; return fluidTypeData.m_name[id]; };
uint32_t FluidType::getViscosity(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_viscosity[id]; };
Density FluidType::getDensity(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_density[id]; };
Step FluidType::getMistDuration(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_mistDuration[id]; };
Distance FluidType::getMaxMistSpread(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_maxMistSpread[id]; };
MaterialTypeId FluidType::getFreezesInto(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_freezesInto[id]; };
float FluidType::getEvaporationRate(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_evaporationRate[id]; };
Temperature FluidType::getFreezingPoint(const FluidTypeId& id)
{
	assert(id.exists());
	const MaterialTypeId& solid = getFreezesInto(id);
	return MaterialType::getMeltingPoint(solid);
}