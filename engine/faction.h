/*
 * A team which actors can belong to. Has shifting alliances and enemies. May or may not be controlled by a player.
 */
#pragma once
#include "config.h"
#include "types.h"
#include <istream>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include <cassert>
struct DeserializationMemo;
struct Faction
{
	std::unordered_set<FactionId> allies;
	std::unordered_set<FactionId> enemies;
	std::wstring name;
	FactionId id = FACTION_ID_MAX;
	Faction(FactionId _id, std::wstring _name) : name(_name), id(_id) { }
	// Default constructor neccesary for json.
	Faction() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Faction, name, allies, enemies);
};
class SimulationHasFactions final
{
	std::vector<Faction> m_factions;
public:
	Faction& createFaction(std::wstring name);
	Faction& getById(FactionId id);
	Faction& byName(std::wstring name);
	[[nodiscard]] const std::vector<Faction>& getAll() const { return m_factions; }
	[[nodiscard]] bool isAlly(FactionId a, FactionId b);
	[[nodiscard]] bool isEnemy(FactionId a, FactionId b);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasFactions, m_factions);
};
