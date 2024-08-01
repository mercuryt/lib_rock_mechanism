#pragma once
#include "config.h"
#include "items/itemQuery.h"
#include "types.h"
#include <string>
class Actor;
class Item;
struct Faction;
struct UniformElement final 
{
	ItemQuery itemQuery;
	Quantity quantity;
	UniformElement(const ItemType& itemType, Quantity quantity = Quantity::create(1), const MaterialType* materialType = nullptr, Quality qualityMin = Quality::create(0));
	[[nodiscard]] bool operator==(const UniformElement& other) const { return &other == this; }
};
struct Uniform final
{
	std::wstring name;
	std::vector<UniformElement> elements;
};
inline void to_json(Json& data, const Uniform* const& uniform){ data = uniform->name; }
class SimulationHasUniformsForFaction final
{
	FactionId m_faction;
	std::unordered_map<std::wstring, Uniform> m_data;
public:
	SimulationHasUniformsForFaction(FactionId faction) : m_faction(faction) { }
	Uniform& createUniform(std::wstring& name, std::vector<UniformElement>& elements);
	void destroyUniform(Uniform& uniform);
	Uniform& at(std::wstring name){ assert(m_data.contains(name)); return m_data.at(name); }
	std::unordered_map<std::wstring, Uniform>& getAll();
};
class SimulationHasUniforms final
{
	FactionIdMap<SimulationHasUniformsForFaction> m_data;
public:
	void registerFaction(FactionId faction) { m_data.try_emplace(faction, faction); }
	void unregisterFaction(FactionId faction) { m_data.erase(faction); }
	SimulationHasUniformsForFaction& at(FactionId faction);
};
