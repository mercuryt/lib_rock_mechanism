#include "faction.h"
Faction& SimulationHasFactions::getById(const FactionId& id)
{
	return m_factions[id];
}
const Faction& SimulationHasFactions::getById(const FactionId& id) const
{
	return m_factions[id];
}
FactionId SimulationHasFactions::createFaction(std::string name)
{
	FactionId id = FactionId::create(m_factions.size());
	m_factions.emplaceBack(id, name);
	return id;
}
FactionId SimulationHasFactions::byName(std::string name)
{
	auto found = std::ranges::find(m_factions, name, &Faction::name);
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
bool SimulationHasFactions::containsFactionWithName(std::string name) const
{
	return std::ranges::contains(m_factions, name, &Faction::name);
}
