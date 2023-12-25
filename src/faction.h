/*
 * A team which actors can belong to. Has shifting alliances and enemies. May or may not be controlled by a player.
 */
#pragma once
#include "config.h"
#include <istream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <cassert>
struct Faction
{
	std::wstring m_name;
	std::unordered_set<Faction*> allies;
	std::unordered_set<Faction*> enemies;
	Faction(std::wstring n) : m_name(n) { }
	[[nodiscard]] bool operator==(const Faction& faction) const { return &faction == this;}
	Faction(const Faction& faction) = delete;
	Faction(Faction&& faction) = delete;
};
inline void to_json(Json& data, const Faction& faction) { data = faction.m_name; }

class SimulationHasFactions final
{
	std::list<Faction> m_factions;
	std::unordered_map<std::wstring, Faction*> m_factionsByName;
public:
	Faction& byName(std::wstring name){ return *m_factionsByName.at(name); }
	Faction& createFaction(std::wstring name){ m_factions.emplace_back(name); assert(!m_factionsByName.contains(name)); m_factionsByName[name] = &m_factions.back(); return m_factions.back(); }
	void destroyFaction(Faction& faction){ assert(faction.allies.empty() && faction.enemies.empty()); m_factionsByName.erase(faction.m_name); m_factions.remove(faction); }
};
