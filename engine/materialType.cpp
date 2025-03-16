#include "materialType.h"
#include <algorithm>
#include <ranges>
MaterialCategoryTypeId MaterialTypeCategory::size() { return MaterialCategoryTypeId::create(materialTypeCategoryData.m_name.size()); }
const MaterialCategoryTypeId MaterialTypeCategory::byName(const std::string&& name)
{
	auto found = materialTypeCategoryData.m_name.find(name);
	assert(found != materialTypeCategoryData.m_name.end());
	return MaterialCategoryTypeId::create(found - materialTypeCategoryData.m_name.begin());
}
void MaterialTypeCategory::create(std::string name) { materialTypeCategoryData.m_name.add(name); }
const std::string& MaterialTypeCategory::getName(MaterialCategoryTypeId id){ return materialTypeCategoryData.m_name[id]; }
SpoilsDataTypeId SpoilData::create(const MaterialTypeId& mt, const ItemTypeId& it, const Percent& c, const Quantity& mi, const Quantity& ma)
{
	spoilData.m_materialType.add(mt);
	spoilData.m_itemType.add(it);
	spoilData.m_chance.add(c);
	spoilData.m_min.add(mi);
	spoilData.m_max.add(ma);
	return SpoilsDataTypeId::create(spoilData.m_materialType.size() - 1);
}
MaterialTypeId SpoilData::getMaterialType(const SpoilsDataTypeId& id) { return spoilData.m_materialType[id]; };
ItemTypeId SpoilData::getItemType(const SpoilsDataTypeId& id) { return spoilData.m_itemType[id]; };
Percent SpoilData::getChance(const SpoilsDataTypeId& id) { return spoilData.m_chance[id]; };
Quantity SpoilData::getMin(const SpoilsDataTypeId& id) { return spoilData.m_min[id]; };
Quantity SpoilData::getMax(const SpoilsDataTypeId& id) { return spoilData.m_max[id]; };
MaterialTypeId MaterialType::byName(const std::string name)
{
	auto found = materialTypeData.m_name.find(name);
	assert(found != materialTypeData.m_name.end());
	return MaterialTypeId::create(found - materialTypeData.m_name.begin());
}
MaterialTypeId MaterialType::create(const MaterialTypeParamaters& p)
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
void MaterialType::setConstructionParamaters(const MaterialTypeId& materialType, const MaterialTypeConstructionDataParamaters& p)
{
	materialTypeData.m_construction_consumed[materialType] = p.consumed;
	materialTypeData.m_construction_unconsumed[materialType] = p.unconsumed;
	materialTypeData.m_construction_byproducts[materialType] = p.byproducts;
	//TODO: Should there be a construction type name here?
	//materialTypeData.m_construction_name[materialType] = p.name;
	materialTypeData.m_construction_skill[materialType] = p.skill;
	materialTypeData.m_construction_duration[materialType] = p.duration;
}
bool MaterialType::empty() { return materialTypeData.m_density.empty(); }
MaterialTypeId MaterialType::size() { return MaterialTypeId::create(materialTypeData.m_name.size()); }
std::string& MaterialType::getName(const MaterialTypeId& id) { return materialTypeData.m_name[id]; };
Density MaterialType::getDensity(const MaterialTypeId& id) { return materialTypeData.m_density[id]; };
uint32_t MaterialType::getHardness(const MaterialTypeId& id) { return materialTypeData.m_hardness[id]; };
bool MaterialType::getTransparent(const MaterialTypeId& id) { return materialTypeData.m_transparent[id]; };
MaterialCategoryTypeId MaterialType::getMaterialTypeCategory(const MaterialTypeId& id) { return materialTypeData.m_materialTypeCategory[id]; };
std::vector<SpoilsDataTypeId>& MaterialType::getSpoilData(const MaterialTypeId& id) { return materialTypeData.m_spoilData[id]; };
Temperature MaterialType::getMeltingPoint(const MaterialTypeId& id) { return materialTypeData.m_meltingPoint[id]; };
FluidTypeId MaterialType::getMeltsInto(const MaterialTypeId& id) { return materialTypeData.m_meltsInto[id]; };
bool MaterialType::canMelt(const MaterialTypeId& id) { return materialTypeData.m_meltsInto[id].exists(); }
// Fire.
bool MaterialType::canBurn(const MaterialTypeId& id) { return materialTypeData.m_burnStageDuration[id].exists(); }
Step MaterialType::getBurnStageDuration(const MaterialTypeId& id) { return materialTypeData.m_burnStageDuration[id]; };
Step MaterialType::getFlameStageDuration(const MaterialTypeId& id) { return materialTypeData.m_flameStageDuration[id]; };
Temperature MaterialType::getIgnitionTemperature(const MaterialTypeId& id) { return materialTypeData.m_ignitionTemperature[id]; };
TemperatureDelta MaterialType::getFlameTemperature(const MaterialTypeId& id) { return materialTypeData.m_flameTemperature[id]; };
// Construct.
std::vector<std::pair<ItemQuery, Quantity>>& MaterialType::construction_getConsumed(const MaterialTypeId& id) { return materialTypeData.m_construction_consumed[id]; };
std::vector<std::pair<ItemQuery, Quantity>>& MaterialType::construction_getUnconsumed(const MaterialTypeId& id) { return materialTypeData.m_construction_unconsumed[id]; };
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>>& MaterialType::construction_getByproducts(const MaterialTypeId& id) { return materialTypeData.m_construction_byproducts[id]; };
SkillTypeId MaterialType::construction_getSkill(const MaterialTypeId& id) { return materialTypeData.m_construction_skill[id]; };
Step MaterialType::construction_getDuration(const MaterialTypeId& id) { return materialTypeData.m_construction_duration[id]; };
