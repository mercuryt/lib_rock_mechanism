#include "uniform.h"
#include "area.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include <chrono>
#include <cinttypes>
#include <math.h>
UniformElement::UniformElement(ItemTypeId itemType, Quantity quantity, MaterialTypeId materialType,[[maybe_unused]] Quality qualityMin) :
	itemQuery(itemType, materialType), quantity(quantity) { }
Uniform& SimulationHasUniformsForFaction::createUniform(std::wstring& name, std::vector<UniformElement>& elements)
{
	auto pair = m_data.try_emplace(name, name, elements);
	assert(pair.second);
	return pair.first->second;
}
void SimulationHasUniformsForFaction::destroyUniform(Uniform& uniform)
{
	m_data.erase(uniform.name);
}
std::unordered_map<std::wstring, Uniform>& SimulationHasUniformsForFaction::getAll(){ return m_data; }
SimulationHasUniformsForFaction& SimulationHasUniforms::at(FactionId faction) 
{ 
	if(!m_data.contains(faction))
		registerFaction(faction);
	return m_data.at(faction); 
}
