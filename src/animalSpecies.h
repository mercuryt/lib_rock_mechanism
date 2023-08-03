#pragma once
#include "shape.h"
#include "moveType.h"
#include "util.h"
#include "body.h"
#include <string>
struct AnimalSpecies
{
	const std::string name;
	const bool sentient;
	// Min, max, newborn.
	const std::array<uint32_t, 3> strength;
	const std::array<uint32_t, 3> dextarity;
	const std::array<uint32_t, 3> agility;
	const std::array<uint32_t, 3> mass;
	const std::array<uint32_t, 3> volume;
	const std::array<uint32_t, 2> deathAge;
	const std::array<uint32_t, 2> adultAge;
	const uint32_t stepsTillDieWithoutFood;
	const uint32_t stepsEatFrequency;
	const uint32_t stepsTillDieWithoutFluid;
	const uint32_t stepsFluidDrinkFreqency;
	const uint32_t stepsTillFullyGrown;
	const uint32_t stepsTillDieInUnsafeTemperature;
	const uint32_t stepsSleepFrequency;
	const uint32_t stepsTillSleepOveride;
	const uint32_t stepsSleepDuration;
	const uint32_t minimumSafeTemperature;
	const uint32_t maximumSafeTemperature;
	const bool eatsMeat;
	const bool eatsLeaves;
	const bool eatsFruit;
	const MaterialType& materialType;
	const MoveType& moveType;
	const FluidType& fluidType;
	const BodyType& bodyType;
	std::vector<const Shape*> shapes;
	const Shape& shapeForPercentGrown(uint32_t percentGrown) const
	{
		size_t index = util::scaleByPercentRange(0, shapes.size() - 1, percentGrown);
		return *shapes.at(index);
	}
	// Infastructure.
	bool operator==(const AnimalSpecies& animalSpecies){ return this == &animalSpecies; }
	inline static std::vector<AnimalSpecies> data;
	static const AnimalSpecies& byName(std::string name)
	{
		auto found = std::ranges::find(data, name, &AnimalSpecies::name);
		assert(found != data.end());
		return *found;
	}
};
