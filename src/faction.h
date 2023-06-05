#pragma once
#include <string>
#include <unordered_set>
template<class DerivedFaction>
class BaseFaction
{
	std::string m_name;
	std::unordered_set<DerivedFaction*> m_allies;
	std::unordered_set<DerivedFaction*> m_enemies;
	DerivedFaction& derived(){ return static_cast<DerivedFaction&>(*this); }
};
