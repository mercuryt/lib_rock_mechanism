#include "faction.h"
#include "deserializationMemo.h"
#include "simulation.h"
Faction& SimulationHasFactions::getById(const FactionId& id)
{
	return m_factions.at(id.get());
}
FactionId SimulationHasFactions::createFaction(std::wstring name)
{ 
	FactionId id = FactionId::create(m_factions.size());
	m_factions.emplace_back(id, name); 
	return m_factions.back().id; 
}
FactionId SimulationHasFactions::byName(std::wstring name)
{
	auto found =  std::ranges::find(m_factions, name, &Faction::name);
	assert(found != m_factions.end());
	return found->id;
}
bool SimulationHasFactions::isAlly(const FactionId& a, const FactionId& b)
{
	return getById(a).allies.contains(b);
}
bool SimulationHasFactions::isEnemy(const FactionId& a, const FactionId& b)
{
	return getById(a).enemies.contains(b);
}
