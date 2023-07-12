/*
 * A team which actors can belong to. Has shifting alliances and enemies. May or may not be controlled by a player.
 */
#pragma once
#include <string>
#include <unordered_set>
struct Faction
{
	std::string m_name;
	std::unordered_set<Faction*> m_allies;
	std::unordered_set<Faction*> m_enemies;
};
