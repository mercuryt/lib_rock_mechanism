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
	UniformElement(const ItemTypeId& itemType, const Quantity quantity = Quantity::create(1), const MaterialTypeId materialType = MaterialTypeId::null(), const Quality qualityMin = Quality::create(0));
	UniformElement(const Json& data, Area& area);
	[[nodiscard]] bool operator==(const UniformElement& other) const { return &other == this; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(UniformElement, itemQuery, quantity);
};
struct Uniform final
{
	std::wstring name;
	std::vector<UniformElement> elements;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(Uniform, name, elements);
	Uniform(std::wstring n, std::vector<UniformElement> e) : name(n), elements(e) { }
	Uniform(const Json& data, Area& area);
};
inline void to_json(Json& data, const Uniform* const& uniform){ data = uniform->name; }
class SimulationHasUniformsForFaction final
{
	FactionId m_faction;
	SmallMap<std::wstring, Uniform> m_data;
public:
	SimulationHasUniformsForFaction(const FactionId& faction) : m_faction(faction) { }
	SimulationHasUniformsForFaction(const Json& data, Area& area);
	Uniform& createUniform(std::wstring& name, std::vector<UniformElement>& elements);
	void destroyUniform(Uniform& uniform);
	Uniform& byName(std::wstring name){ assert(m_data.contains(name)); return m_data[name]; }
	SmallMap<std::wstring, Uniform>& getAll();
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(SimulationHasUniformsForFaction, m_faction, m_data);
};
class SimulationHasUniforms final
{
	FactionIdMap<SimulationHasUniformsForFaction> m_data;
public:
	SimulationHasUniforms() = default;
	SimulationHasUniforms(const Json& data, Area& area);
	void registerFaction(const FactionId& faction) { m_data.emplace(faction, faction); }
	void unregisterFaction(const FactionId& faction) { m_data.erase(faction); }
	SimulationHasUniformsForFaction& getForFaction(const FactionId& faction);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE_ONLY_SERIALIZE(SimulationHasUniforms, m_data);
};