#pragma once
#include <string>
#include <unordered_set>
struct Faction
{
	std::string m_name;
	std::unordered_set<Faction*> m_allies;
	std::unordered_set<Faction*> m_enemies;
};
