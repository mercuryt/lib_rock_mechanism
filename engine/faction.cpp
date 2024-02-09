#include "faction.h"
#include "deserializationMemo.h"
#include "simulation.h"
Faction::Faction(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo) : name(data["name"].get<std::wstring>()) { }
void Faction::load(const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo)
{
	for(const Json& name : data["allies"])
		allies.insert(&deserializationMemo.m_simulation.m_hasFactions.byName(name.get<std::wstring>()));
	for(const Json& name : data["enemies"])
		enemies.insert(&deserializationMemo.m_simulation.m_hasFactions.byName(name.get<std::wstring>()));
}
Faction& SimulationHasFactions::byName(std::wstring name){ assert(m_factionsByName.contains(name)); return *m_factionsByName.at(name); }
Faction& SimulationHasFactions::createFaction(std::wstring name)
{ 
	m_factions.emplace_back(name); 
	assert(!m_factionsByName.contains(name)); 
	m_factionsByName[name] = &m_factions.back(); 
	return m_factions.back(); 
}
void SimulationHasFactions::destroyFaction(Faction& faction)
{ 
	assert(faction.allies.empty() && faction.enemies.empty()); 
	m_factionsByName.erase(faction.name);
	m_factions.remove(faction); 
}
Json SimulationHasFactions::toJson() const
{
	Json data{{"factions", Json::array()}};
	for(const Faction& faction : m_factions)
		data["factions"].push_back(faction);
	return data;
}
void SimulationHasFactions::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	// Two stage load, first create all factions and then record allies and enemies.
	for(const Json& faction : data["factions"])
	{
		m_factions.emplace_back(faction, deserializationMemo);
		std::wstring name  = m_factions.back().name;
		m_factionsByName[name] = &m_factions.back();
	}
	for(const Json& factionData : data["factions"])
	{
		Faction& faction = byName(factionData["name"].get<std::wstring>());
		faction.load(factionData, deserializationMemo);
	}

}
