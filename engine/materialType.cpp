#include "materialType.h"
#include <algorithm>
#include <ranges>
const MaterialCategoryTypeId MaterialTypeCategory::byName(const std::string&& name)
{
	auto found = materialTypeCategoryData.m_name.find(name);
	assert(found != materialTypeCategoryData.m_name.end());
	return MaterialCategoryTypeId::create(found - materialTypeCategoryData.m_name.begin());
}
void MaterialTypeCategory::create(std::string name) { materialTypeCategoryData.m_name.add(name); }
const std::string& MaterialTypeCategory::getName(MaterialCategoryTypeId id){ return materialTypeCategoryData.m_name[id]; }
SpoilsDataTypeId SpoilData::create(const MaterialTypeId mt, const ItemTypeId it, const double c, const Quantity mi, const Quantity ma)
{
	spoilData.m_materialType.add(mt);
	spoilData.m_itemType.add(it);
	spoilData.m_chance.add(c);
	spoilData.m_min.add(mi);
	spoilData.m_max.add(ma);
	return SpoilsDataTypeId::create(spoilData.m_materialType.size() - 1);
}
MaterialTypeId SpoilData::getMaterialType(SpoilsDataTypeId id) { return spoilData.m_materialType[id]; };
ItemTypeId SpoilData::getItemType(SpoilsDataTypeId id) { return spoilData.m_itemType[id]; };
double SpoilData::getChance(SpoilsDataTypeId id) { return spoilData.m_chance[id]; };
Quantity SpoilData::getMin(SpoilsDataTypeId id) { return spoilData.m_min[id]; };
Quantity SpoilData::getMax(SpoilsDataTypeId id) { return spoilData.m_max[id]; };
MaterialTypeId MaterialType::byName(const std::string name)
{
	auto found = materialTypeData.m_name.find(name);
	assert(found != materialTypeData.m_name.end());
	return MaterialTypeId::create(found - materialTypeData.m_name.begin());
}
MaterialTypeId MaterialType::create(MaterialTypeParamaters& p)
{
	materialTypeData.m_name.add(p.name);
	materialTypeData.m_density.add(p.density);
	materialTypeData.m_hardness.add(p.hardness);
	materialTypeData.m_transparent.add(p.transparent);
	materialTypeData.m_materialTypeCategory.add(p.materialTypeCategory);
	materialTypeData.m_spoilData.add(p.spoilData);
	materialTypeData.m_meltingPoint.add(p.meltingPoint);
	materialTypeData.m_meltsInto.add(p.meltsInto);
	materialTypeData.m_burnStageDuration.add(p.burnStageDuration); 
	materialTypeData.m_flameStageDuration.add(p.flameStageDuration); 
	materialTypeData.m_ignitionTemperature.add(p.ignitionTemperature); 
	materialTypeData.m_flameTemperature.add(p.flameTemperature); 
	materialTypeData.m_construction_consumed.add(p.construction_consumed);
	materialTypeData.m_construction_unconsumed.add(p.construction_unconsumed);
	materialTypeData.m_construction_byproducts.add(p.construction_byproducts);
	materialTypeData.m_construction_skill.add(p.construction_skill);
	materialTypeData.m_construction_duration.add(p.construction_duration);
	return MaterialTypeId::create(materialTypeData.m_name.size() - 1);
}
bool MaterialType::empty() { return materialTypeData.m_density.empty(); }
std::string& MaterialType::getName(MaterialTypeId id) { return materialTypeData.m_name[id]; };
Density MaterialType::getDensity(MaterialTypeId id) { return materialTypeData.m_density[id]; };
uint32_t MaterialType::getHardness(MaterialTypeId id) { return materialTypeData.m_hardness[id]; };
bool MaterialType::getTransparent(MaterialTypeId id) { return materialTypeData.m_transparent[id]; };
MaterialCategoryTypeId MaterialType::getMaterialTypeCategory(MaterialTypeId id) { return materialTypeData.m_materialTypeCategory[id]; };
std::vector<SpoilsDataTypeId>& MaterialType::getSpoilData(MaterialTypeId id) { return materialTypeData.m_spoilData[id]; };
Temperature MaterialType::getMeltingPoint(MaterialTypeId id) { return materialTypeData.m_meltingPoint[id]; };
FluidTypeId MaterialType::getMeltsInto(MaterialTypeId id) { return materialTypeData.m_meltsInto[id]; };
// Fire.
bool MaterialType::canBurn(MaterialTypeId id) { return materialTypeData.m_burnStageDuration[id].empty(); }
Step MaterialType::getBurnStageDuration(MaterialTypeId id) { return materialTypeData.m_burnStageDuration[id]; };
Step MaterialType::getFlameStageDuration(MaterialTypeId id) { return materialTypeData.m_flameStageDuration[id]; };
Temperature MaterialType::getIgnitionTemperature(MaterialTypeId id) { return materialTypeData.m_ignitionTemperature[id]; };
TemperatureDelta MaterialType::getFlameTemperature(MaterialTypeId id) { return materialTypeData.m_flameTemperature[id]; };
// Construct.
std::vector<std::pair<ItemQuery, Quantity>>& MaterialType::construction_getConsumed(MaterialTypeId id) { return materialTypeData.m_construction_consumed[id]; };
std::vector<std::pair<ItemQuery, Quantity>>& MaterialType::construction_getUnconsumed(MaterialTypeId id) { return materialTypeData.m_construction_unconsumed[id]; };
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>>& MaterialType::construction_getByproducts(MaterialTypeId id) { return materialTypeData.m_construction_byproducts[id]; };
SkillTypeId MaterialType::construction_getSkill(MaterialTypeId id) { return materialTypeData.m_construction_skill[id]; };
Step MaterialType::construction_getDuration(MaterialTypeId id) { return materialTypeData.m_construction_duration[id]; };
