#pragma once
#include "shape.h"
#include "moveType.h"
#include <string>
struct AnimalSpecies
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
	// Infastructure.
	bool operator==(const AnimalSpecies& animalSpecies){ return this == &animalSpecies; }
	static std::vector<AnimalSpecies> data;
	static const AnimalSpecies& byName(std::string name)
	{
		auto found = std::ranges::find(data, name, &AnimalSpecies::name);
		assert(found != data.end());
		return *found;
	}
};
