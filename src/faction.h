/*
 * A team which actors can belong to. Has shifting alliances and enemies. May or may not be controlled by a player.
 */
#pragma once
#include <string>
#include <unordered_set>
struct Faction
{
	std::wstring m_name;
	std::unordered_set<Faction*> allies;
	std::unordered_set<Faction*> enemies;
	Faction(std::wstring n) : m_name(n) { }
};
