#pragma once
#include "config.h"
#include "items/itemQuery.h"
#include "types.h"
#include "vectorContainers.h"
#include <string>
class Actor;
class Item;
struct Faction;
struct UniformElement final 
{
	ItemQuery itemQuery;
	Quantity quantity;
	[[nodiscard]] bool operator==(const UniformElement& other) const { return &other == this; }
	static UniformElement create(const ItemTypeId& itemType, const Quantity quantity = Quantity::create(1), const MaterialTypeId materialType = MaterialTypeId::null(), const Quality qualityMin = Quality::create(0));
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(UniformElement, itemQuery, quantity);
};
struct Uniform final
{
	std::wstring name;
	std::vector<UniformElement> elements;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Uniform, name, elements);
};
inline void to_json(Json& data, const Uniform* const& uniform){ data = uniform->name; }
class SimulationHasUniformsForFaction final
{
	FactionId m_faction;
	SmallMap<std::wstring, Uniform> m_data;
public:
	SimulationHasUniformsForFaction(FactionId faction) : m_faction(faction) { }
	Uniform& createUniform(std::wstring& name, std::vector<UniformElement>& elements);
	void destroyUniform(Uniform& uniform);
	Uniform& byName(std::wstring name){ assert(m_data.contains(name)); return m_data[name]; }
	SmallMap<std::wstring, Uniform>& getAll();
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasUniformsForFaction, m_faction, m_data);
};
class SimulationHasUniforms final
{
	FactionIdMap<SimulationHasUniformsForFaction> m_data;
public:
	void registerFaction(const FactionId& faction) { m_data.emplace(faction, faction); }
	void unregisterFaction(const FactionId& faction) { m_data.erase(faction); }
	SimulationHasUniformsForFaction& getForFaction(const FactionId& faction);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(SimulationHasUniforms, m_data);
};