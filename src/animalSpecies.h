#pragma once
#include "moveEvent.h"
#include "shape.h"
#include "moveType.h"
#include <string>
class AnimalSpecies
{
	std::string name;
	bool sentient;
	// Min, max, newborn.
	std::array<uint32_t, 3> strength;
	std::array<uint32_t, 3> dextarity;
	std::array<uint32_t, 3> agility;
	std::array<uint32_t, 3> mass;
	std::array<uint32_t, 3> volume;
	std::array<uint32_t, 2> deathAge;
	std::array<uint32_t, 2> adultSizeAge;
	const MoveType& moveType;
	const Shape& shape;
};
