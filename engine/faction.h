/*
 * A team which actors can belong to. Has shifting alliances and enemies. May or may not be controlled by a player.
 */
#pragma once
#include "numericTypes/types.h"
#include <string>
struct DeserializationMemo;
struct Faction
{
	SmallSet<FactionId> allies;
	SmallSet<FactionId> enemies;
	std::string name;
	FactionId id;
	Faction(const FactionId& _id, std::string _name) : name(_name), id(_id) { }
	// Default constructor neccesary for json.
	Faction() = default;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Faction, id, name, allies, enemies);
};
class SimulationHasFactions final
{
	// Factions are never destroyed so their id is their index.
	std::vector<Faction> m_factions;
public:
	Faction& getById(const FactionId& id);
	const Faction& getById(const FactionId& id) const;
	FactionId createFaction(std::string name);
	FactionId byName(std::string name);
	[[nodiscard]] bool isAlly(const FactionId& a, const FactionId& b) const;
	[[nodiscard]] bool isEnemy(const FactionId& a, const FactionId& b) const;
	[[nodiscard]] bool containsFactionWithName(std::string name) const;
	[[nodiscard]] std::vector<Faction>& getAll() { return m_factions; }
	[[nodiscard]] const std::vector<Faction>& getAll() const { return m_factions; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasFactions, m_factions);
};
