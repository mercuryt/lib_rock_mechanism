#include "faction.h"
#include "deserializationMemo.h"
#include "simulation.h"
#include "types.h"
Faction& SimulationHasFactions::createFaction(std::wstring name)
{ 
	FactionId id = FactionId::create(m_factions.size());
	m_factions.emplace_back(id, name); 
	return m_factions.back(); 
}
Faction& SimulationHasFactions::getById(FactionId id)
{
	return m_factions.at(id.get());
}
Faction& SimulationHasFactions::byName(std::wstring name)
{
	return *std::ranges::find(m_factions, name, &Faction::name);
}
bool SimulationHasFactions::isAlly(FactionId a, FactionId b)
{
	return getById(a).allies.contains(b);
}
bool SimulationHasFactions::isEnemy(FactionId a, FactionId b)
{
	return getById(a).enemies.contains(b);
}
