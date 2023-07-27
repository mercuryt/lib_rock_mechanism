#pragma once
#include "shape.h"
#include "moveType.h"
#include "util.h"
#include "body.h"
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
	std::array<uint32_t, 2> adultAge;
	uint32_t stepsTillDieWithoutFood;
	uint32_t stepsTillFullyGrown;
	uint32_t stepsSleepFrequency;
	uint32_t stepsTillSleepOveride;
	uint32_t stepsSleepDuration;
	const MoveType& moveType;
	const FluidType& fluidType;
	std::vector<const Shape*> shapes;
	BodyType& bodyType;
	const Shape& shapeForPercentGrown(uint32_t percentGrown) const
	{
		size_t index = util::scaleByPercentRange(0, shapes.size() - 1, percentGrown);
		return *shapes.at(index);
	}
	const bool eatsMeat;
	const bool eatsLeaves;
	const bool eatsFruit;
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
