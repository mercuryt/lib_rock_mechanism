/*
 * Represents a human player.
 */
#pragma once

#include "faction.h"

#include <string>

class Player
{
	inline static uint32_t nextId = 1;
public:
	std::string m_name;
	uint32_t m_id;
	Faction* m_faction;
	Player(std::string n) : m_name(n), m_id(nextId++) { }
};
