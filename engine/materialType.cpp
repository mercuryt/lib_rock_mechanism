#include "materialType.h"
#include <algorithm>
#include <ranges>
const MaterialCategoryTypeId MaterialTypeCategory::byName(const std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return MaterialCategoryTypeId::create(found - data.m_name.begin());
}
MaterialTypeId MaterialType::byName(const std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return MaterialTypeId::create(found - data.m_name.begin());
}
MaterialTypeId MaterialType::create(MaterialTypeParamaters& p)
{
	data.m_name.add(p.name);
	data.m_density.add(p.density);
	data.m_hardness.add(p.hardness);
	data.m_transparent.add(p.transparent);
	data.m_materialTypeCategory.add(p.materialTypeCategory);
	data.m_spoilData.add(p.spoilData);
	data.m_meltingPoint.add(p.meltingPoint);
	data.m_meltsInto.add(p.meltsInto);
	data.m_burnStageDuration.add(p.burnStageDuration); 
	data.m_flameStageDuration.add(p.flameStageDuration); 
	data.m_ignitionTemperature.add(p.ignitionTemperature); 
	data.m_flameTemperature.add(p.flameTemperature); 
	data.m_construction_consumed.add(p.construction_consumed);
	data.m_construction_unconsumed.add(p.construction_unconsumed);
	data.m_construction_byproducts.add(p.construction_byproducts);
	data.m_construction_skill.add(p.construction_skill);
	data.m_construction_duration.add(p.construction_duration);
	return MaterialTypeId::create(data.m_name.size() - 1);
}
