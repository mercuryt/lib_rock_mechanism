#include "uniform.h"
#include "area.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include <chrono>
#include <cinttypes>
#include <math.h>
UniformElement UniformElement::create(const ItemTypeId& itemType, const Quantity quantity, const MaterialTypeId materialType,[[maybe_unused]] const Quality qualityMin)
{
	return {
		.itemQuery = ItemQuery::create(itemType, materialType),
		.quantity = quantity
	};
}
Uniform& SimulationHasUniformsForFaction::createUniform(std::wstring& name, std::vector<UniformElement>& elements)
{
	return m_data.emplace(name, name, elements);
}
void SimulationHasUniformsForFaction::destroyUniform(Uniform& uniform)
{
	m_data.erase(uniform.name);
}
SmallMap<std::wstring, Uniform>& SimulationHasUniformsForFaction::getAll(){ return m_data; }
SimulationHasUniformsForFaction& SimulationHasUniforms::getForFaction(const FactionId& faction) 
{ 
	if(!m_data.contains(faction))
		registerFaction(faction);
	return m_data[faction]; 
}