#include "faction.h"
#include "deserializationMemo.h"
#include "simulation/simulation.h"
Faction& SimulationHasFactions::getById(const FactionId& id)
{
	return m_factions.at(id.get());
}
const Faction& SimulationHasFactions::getById(const FactionId& id) const
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
bool SimulationHasFactions::isAlly(const FactionId& a, const FactionId& b) const
{
	return getById(a).allies.contains(b);
}
bool SimulationHasFactions::isEnemy(const FactionId& a, const FactionId& b) const
{
	return getById(a).enemies.contains(b);
}
bool SimulationHasFactions::containsFactionWithName(std::wstring name) const
{
	return std::ranges::contains(m_factions, name, &Faction::name);
}
