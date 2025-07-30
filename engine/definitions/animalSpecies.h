#pragma once
#include "../numericTypes/types.h"
#include "../body.h"
#include "../dataStructures/strongVector.h"
#include <string>
struct AnimalSpeciesParamaters
{
	std::string name;
	bool sentient;
	// Min, max, newborn.
	std::array<AttributeLevel, 3> strength;
	std::array<AttributeLevel, 3> dextarity;
	std::array<AttributeLevel, 3> agility;
	std::array<Mass, 3> mass;
	std::array<uint32_t, 3> height;
	std::array<Step, 2> deathAgeSteps;
	Step stepsTillFullyGrown;
	Step stepsTillDieWithoutFood;
	Step stepsEatFrequency;
	Step stepsTillDieWithoutFluid;
	Step stepsFluidDrinkFrequency;
	Step stepsTillDieInUnsafeTemperature;
	Temperature minimumSafeTemperature;
	Temperature maximumSafeTemperature;
	Step stepsSleepFrequency;
	Step stepsTillSleepOveride;
	Step stepsSleepDuration;
	bool nocturnal;
	bool eatsMeat;
	bool eatsLeaves;
	bool eatsFruit;
	Distance visionRange;
	uint32_t bodyScale;
	MaterialTypeId materialType;
	MoveTypeId moveType;
	FluidTypeId fluidType;
	BodyTypeId bodyType;
	std::vector<ShapeId> shapes = {};
};
class AnimalSpecies
{
	StrongVector<std::string, AnimalSpeciesId> m_name;
	StrongBitSet<AnimalSpeciesId> m_sentient;
	// Min, max, newborn.
	StrongVector<std::array<AttributeLevel, 3>, AnimalSpeciesId> m_strength;
	StrongVector<std::array<AttributeLevel, 3>, AnimalSpeciesId> m_dextarity;
	StrongVector<std::array<AttributeLevel, 3>, AnimalSpeciesId> m_agility;
	StrongVector<std::array<Mass, 3>, AnimalSpeciesId> m_mass;
	StrongVector<std::array<uint32_t, 3>, AnimalSpeciesId> m_height;
	StrongVector<std::array<Step, 2>, AnimalSpeciesId> m_deathAgeSteps;
	StrongVector<Step, AnimalSpeciesId> m_stepsTillFullyGrown;
	StrongVector<Step, AnimalSpeciesId> m_stepsTillDieWithoutFood;
	StrongVector<Step, AnimalSpeciesId> m_stepsEatFrequency;
	StrongVector<Step, AnimalSpeciesId> m_stepsTillDieWithoutFluid;
	StrongVector<Step, AnimalSpeciesId> m_stepsFluidDrinkFrequency;
	StrongVector<Step, AnimalSpeciesId> m_stepsTillDieInUnsafeTemperature;
	StrongVector<Temperature, AnimalSpeciesId> m_minimumSafeTemperature;
	StrongVector<Temperature, AnimalSpeciesId> m_maximumSafeTemperature;
	StrongVector<Step, AnimalSpeciesId> m_stepsSleepFrequency;
	StrongVector<Step, AnimalSpeciesId> m_stepsTillSleepOveride;
	StrongVector<Step, AnimalSpeciesId> m_stepsSleepDuration;
	StrongBitSet<AnimalSpeciesId> m_nocturnal;
	StrongBitSet<AnimalSpeciesId> m_eatsMeat;
	StrongBitSet<AnimalSpeciesId> m_eatsLeaves;
	StrongBitSet<AnimalSpeciesId> m_eatsFruit;
	StrongVector<Distance, AnimalSpeciesId> m_visionRange;
	StrongVector<uint32_t, AnimalSpeciesId> m_bodyScale;
	StrongVector<MaterialTypeId, AnimalSpeciesId> m_solid;
	StrongVector<MoveTypeId, AnimalSpeciesId> m_moveType;
	StrongVector<FluidTypeId, AnimalSpeciesId> m_fluidType;
	StrongVector<BodyTypeId, AnimalSpeciesId> m_bodyType;
	StrongVector<std::vector<ShapeId>, AnimalSpeciesId> m_shapes;
public:
	static AnimalSpeciesId create(AnimalSpeciesParamaters);
	[[nodiscard]] static ShapeId shapeForPercentGrown(const AnimalSpeciesId& id, const Percent& percentGrown);
	[[nodiscard]] static AnimalSpeciesId byName(std::string name);
	[[nodiscard]] static std::string getName(const AnimalSpeciesId& id);
	[[nodiscard]] static bool getSentient(const AnimalSpeciesId& id);
	[[nodiscard]] static std::array<AttributeLevel, 3> getStrength(const AnimalSpeciesId& id);
	[[nodiscard]] static std::array<AttributeLevel, 3> getDextarity(const AnimalSpeciesId& id);
	[[nodiscard]] static std::array<AttributeLevel, 3> getAgility(const AnimalSpeciesId& id);
	[[nodiscard]] static std::array<Mass, 3> getMass(const AnimalSpeciesId& id);
	[[nodiscard]] static std::array<uint32_t, 3> getHeight(const AnimalSpeciesId& id);
	[[nodiscard]] static std::array<Step, 2> getDeathAgeSteps(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsTillFullyGrown(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsTillDieWithoutFood(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsEatFrequency(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsTillDieWithoutFluid(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsFluidDrinkFrequency(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsTillDieInUnsafeTemperature(const AnimalSpeciesId& id);
	[[nodiscard]] static Temperature getMinimumSafeTemperature(const AnimalSpeciesId& id);
	[[nodiscard]] static Temperature getMaximumSafeTemperature(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsSleepFrequency(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsTillSleepOveride(const AnimalSpeciesId& id);
	[[nodiscard]] static Step getStepsSleepDuration(const AnimalSpeciesId& id);
	[[nodiscard]] static bool getNocturnal(const AnimalSpeciesId& id);
	[[nodiscard]] static bool getEatsMeat(const AnimalSpeciesId& id);
	[[nodiscard]] static bool getEatsLeaves(const AnimalSpeciesId& id);
	[[nodiscard]] static bool getEatsFruit(const AnimalSpeciesId& id);
	[[nodiscard]] static Distance getVisionRange(const AnimalSpeciesId& id);
	[[nodiscard]] static uint32_t getBodyScale(const AnimalSpeciesId& id);
	[[nodiscard]] static MaterialTypeId getMaterialType(const AnimalSpeciesId& id);
	[[nodiscard]] static MoveTypeId getMoveType(const AnimalSpeciesId& id);
	[[nodiscard]] static FluidTypeId getFluidType(const AnimalSpeciesId& id);
	[[nodiscard]] static BodyTypeId getBodyType(const AnimalSpeciesId& id);
	[[nodiscard]] static std::vector<ShapeId> getShapes(const AnimalSpeciesId& id);
	[[nodiscard]] static AnimalSpeciesId size();
};
inline AnimalSpecies animalSpeciesData;
