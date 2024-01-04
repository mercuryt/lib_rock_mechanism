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
	const std::array<Mass, 3> mass;
	const std::array<uint32_t, 2> deathAgeSteps;
	const Step stepsTillFullyGrown;
	const Step stepsTillDieWithoutFood;
	const Step stepsEatFrequency;
	const Step stepsTillDieWithoutFluid;
	const Step stepsFluidDrinkFreqency;
	const Step stepsTillDieInUnsafeTemperature;
	const Temperature minimumSafeTemperature;
	const Temperature maximumSafeTemperature;
	const Step stepsSleepFrequency;
	const Step stepsTillSleepOveride;
	const Step stepsSleepDuration;
	const bool nocturnal;
	const bool eatsMeat;
	const bool eatsLeaves;
	const bool eatsFruit;
	const uint32_t visionRange;
	uint32_t bodyScale;
	const MaterialType& materialType;
	const MoveType& moveType;
	const FluidType& fluidType;
	const BodyType& bodyType;
	std::vector<const Shape*> shapes;
	const Shape& shapeForPercentGrown(Percent percentGrown) const
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
inline void to_json(Json& data, const AnimalSpecies* const& species){ data = species->name; }
inline void to_json(Json& data, const AnimalSpecies& species){ data = species.name; }
inline void from_json(const Json& data, const AnimalSpecies*& species){ species = &AnimalSpecies::byName(data.get<std::string>()); }
