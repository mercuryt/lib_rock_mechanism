/*
 * Represents a human player.
 */
#pragma once

#include "faction.h"

#include <string>

class Player
{
	inline static int32_t nextId = 1;
public:
	std::string m_name;
	int32_t m_id;
	Faction* m_faction;
	Player(std::string n) : m_name(n), m_id(nextId++) { }
};
