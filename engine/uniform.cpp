#include "uniform.h"
#include "area.h"
#include "items/items.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include <chrono>
#include <cinttypes>
#include <math.h>
UniformElement UniformElement::create(const ItemTypeId& itemType, const Quantity& quantity, const MaterialTypeId& materialType, const MaterialCategoryTypeId& materialCategoryType, [[maybe_unused]] const Quality qualityMin)
{
	assert(itemType.exists());
	UniformElement output{
		.m_itemType = itemType,
		.m_materialCategoryType = materialCategoryType,
		.m_materialType = materialType,
		.m_quantity = quantity,
	};
	return output;
}
bool UniformElement::query(const ItemIndex& item, const Items& items) const
{
	assert(m_itemType.exists());
	if(items.getItemType(item) != m_itemType)
		return false;
	const MaterialTypeId& materialType = items.getMaterialType(item);
	if(m_materialType.exists() && materialType != m_materialType)
		return false;
	if(m_materialCategoryType.exists() && MaterialType::getMaterialTypeCategory(materialType) != m_materialCategoryType)
		return false;
	return true;
}
Uniform& SimulationHasUniformsForFaction::createUniform(std::string& name, std::vector<UniformElement>& elements)
{
	return m_data.emplace(name, name, elements);
}
void SimulationHasUniformsForFaction::destroyUniform(Uniform& uniform)
{
	m_data.erase(uniform.name);
}
SmallMap<std::string, Uniform>& SimulationHasUniformsForFaction::getAll(){ return m_data; }
SimulationHasUniformsForFaction& SimulationHasUniforms::getForFaction(const FactionId& faction) 
{ 
	if(!m_data.contains(faction))
		registerFaction(faction);
	return m_data[faction]; 
}