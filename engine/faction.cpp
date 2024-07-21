#include "faction.h"
#include "deserializationMemo.h"
#include "simulation.h"
Faction& SimulationHasFactions::createFaction(std::wstring name)
{ 
	FactionId id = m_factions.size();
	m_factions.emplace_back(id, name); 
	return m_factions.back(); 
}
Faction& SimulationHasFactions::getById(FactionId id)
{
	return m_factions.at(id);
}
