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
struct DeserializationMemo;
struct Faction
{
	std::wstring name;
	std::unordered_set<Faction*> allies;
	std::unordered_set<Faction*> enemies;
	Faction(std::wstring n) : name(n) { }
	[[nodiscard]] bool operator==(const Faction& faction) const { return &faction == this;}
	Faction(const Faction& faction) = delete;
	Faction(Faction&& faction) = delete;
	Faction(const Json& data, DeserializationMemo& deserializationMemo);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
};
inline void to_json(Json& data, const Faction* const& faction) { data = faction->name; }
inline void to_json(Json& data, Faction* const& faction) { data = faction->name; }
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_ONLY_SERIALIZE(Faction, name, allies, enemies);

class SimulationHasFactions final
{
	std::list<Faction> m_factions;
	std::unordered_map<std::wstring, Faction*> m_factionsByName;
public:
	Faction& byName(std::wstring name);
	Faction& createFaction(std::wstring name);
	void destroyFaction(Faction& faction);
	Json toJson() const;
	void load(const Json& data, DeserializationMemo& deserializationMemo);
};
