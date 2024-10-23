/*
 * A team which actors can belong to. Has shifting alliances and enemies. May or may not be controlled by a player.
 */
#pragma once
#include "types.h"
#include <string>
struct DeserializationMemo;
struct Faction
{
	FactionIdSet allies;
	FactionIdSet enemies;
	std::wstring name;
	FactionId id;
	Faction(const FactionId& _id, std::wstring _name) : name(_name), id(_id) { }
	// Default constructor neccesary for json.
	Faction() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Faction, id, name, allies, enemies);
};
class SimulationHasFactions final
{
	// Factions are never destroyed so their id is their index.
	std::vector<Faction> m_factions;
	Faction& getById(const FactionId& id);
public:
	FactionId createFaction(std::wstring name);
	FactionId byName(std::wstring name);
	[[nodiscard]] bool isAlly(const FactionId& a, const FactionId& b);
	[[nodiscard]] bool isEnemy(const FactionId& a, const FactionId& b);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasFactions, m_factions);
};
