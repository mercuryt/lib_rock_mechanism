#include "uniform.h"
#include "area.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include <chrono>
#include <cinttypes>
#include <math.h>
UniformElement::UniformElement(const ItemTypeId& itemType, const Quantity quantity, const MaterialTypeId materialType,[[maybe_unused]] const Quality qualityMin) :
	itemQuery(itemType, materialType),
	quantity(quantity) { }
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
//TODO: these json constructors are just to pass Area into ItemQuery to link the reference, they can be removed when reference is changed to not use pointer.
UniformElement::UniformElement(const Json& data, Area& area) :
	itemQuery(data["itemQuery"], area),
	quantity(data["quantity"].get<Quantity>()) { }
Uniform::Uniform(const Json& data, Area& area) :
	name(data["name"].get<std::wstring>())
{
	elements.reserve(data["elements"].size());
	for(const Json& element : data["elements"])
		elements.emplace_back(element, area);
}
SimulationHasUniformsForFaction::SimulationHasUniformsForFaction(const Json& data, Area& area) :
	m_faction(data["m_faction"].get<FactionId>())
{
	for(const Json& pair : data["m_data"])
	{
		std::wstring name = pair[0].get<std::wstring>();
		m_data.emplace(name, pair[1], area);
	}
}
SimulationHasUniforms::SimulationHasUniforms(const Json& data, Area& area)
{
	for(const Json& pair : data["m_data"])
	{
		FactionId faction = pair[0].get<FactionId>();
		m_data.emplace(faction, pair[1], area);
	}
}