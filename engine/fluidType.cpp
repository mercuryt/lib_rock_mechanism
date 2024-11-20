#include "fluidType.h"
const FluidTypeId FluidType::byName(const std::string name)
{
	auto found = fluidTypeData.m_name.find(name);
	assert(found != fluidTypeData.m_name.end());
	return FluidTypeId::create(found - fluidTypeData.m_name.begin());
}
// Static method.
FluidTypeId FluidType::create(FluidTypeParamaters& p)
{
	fluidTypeData.m_name.add(p.name);
	fluidTypeData.m_viscosity.add(p.viscosity);
	fluidTypeData.m_density.add(p.density);
	fluidTypeData.m_mistDuration.add(p.mistDuration);
	fluidTypeData.m_maxMistSpread.add(p.maxMistSpread);
	fluidTypeData.m_freezesInto.add(p.freezesInto);
	return FluidTypeId::create(fluidTypeData.m_name.size() - 1);
}
// Static method.
void FluidType::setFreezesInto(const FluidTypeId& fluid, const MaterialTypeId& material) { assert(fluid.exists()); assert(material.exists()); fluidTypeData.m_freezesInto[fluid] = material; }
std::string FluidType::getName(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_name[id]; };
uint32_t FluidType::getViscosity(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_viscosity[id]; };
Density FluidType::getDensity(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_density[id]; };
Step FluidType::getMistDuration(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_mistDuration[id]; };
DistanceInBlocks FluidType::getMaxMistSpread(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_maxMistSpread[id]; };
MaterialTypeId FluidType::getFreezesInto(const FluidTypeId& id) { assert(id.exists()); return fluidTypeData.m_freezesInto[id]; };
