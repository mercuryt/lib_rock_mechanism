#include "fluidType.h"
const FluidTypeId FluidType::byName(const std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return FluidTypeId::create(found - data.m_name.begin());
}
