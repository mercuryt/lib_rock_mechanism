#pragma once
#include "types.h"
#include "body.h"
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
	DistanceInBlocks visionRange;
	uint32_t bodyScale;
	MaterialTypeId materialType;
	MoveTypeId moveType;
	FluidTypeId fluidType;
	BodyTypeId bodyType;
	std::vector<ShapeId> shapes = {};
};
class AnimalSpecies
{
	DataVector<std::string, AnimalSpeciesId> m_name;
	DataBitSet<AnimalSpeciesId> m_sentient;
	// Min, max, newborn.
	DataVector<std::array<AttributeLevel, 3>, AnimalSpeciesId> m_strength;
	DataVector<std::array<AttributeLevel, 3>, AnimalSpeciesId> m_dextarity;
	DataVector<std::array<AttributeLevel, 3>, AnimalSpeciesId> m_agility;
	DataVector<std::array<Mass, 3>, AnimalSpeciesId> m_mass;
	DataVector<std::array<Step, 2>, AnimalSpeciesId> m_deathAgeSteps;
	DataVector<Step, AnimalSpeciesId> m_stepsTillFullyGrown;
	DataVector<Step, AnimalSpeciesId> m_stepsTillDieWithoutFood;
	DataVector<Step, AnimalSpeciesId> m_stepsEatFrequency;
	DataVector<Step, AnimalSpeciesId> m_stepsTillDieWithoutFluid;
	DataVector<Step, AnimalSpeciesId> m_stepsFluidDrinkFrequency;
	DataVector<Step, AnimalSpeciesId> m_stepsTillDieInUnsafeTemperature;
	DataVector<Temperature, AnimalSpeciesId> m_minimumSafeTemperature;
	DataVector<Temperature, AnimalSpeciesId> m_maximumSafeTemperature;
	DataVector<Step, AnimalSpeciesId> m_stepsSleepFrequency;
	DataVector<Step, AnimalSpeciesId> m_stepsTillSleepOveride;
	DataVector<Step, AnimalSpeciesId> m_stepsSleepDuration;
	DataBitSet<AnimalSpeciesId> m_nocturnal;
	DataBitSet<AnimalSpeciesId> m_eatsMeat;
	DataBitSet<AnimalSpeciesId> m_eatsLeaves;
	DataBitSet<AnimalSpeciesId> m_eatsFruit;
	DataVector<DistanceInBlocks, AnimalSpeciesId> m_visionRange;
	DataVector<uint32_t, AnimalSpeciesId> m_bodyScale;
	DataVector<MaterialTypeId, AnimalSpeciesId> m_materialType;
	DataVector<MoveTypeId, AnimalSpeciesId> m_moveType;
	DataVector<FluidTypeId, AnimalSpeciesId> m_fluidType;
	DataVector<BodyTypeId, AnimalSpeciesId> m_bodyType;
	DataVector<std::vector<ShapeId>, AnimalSpeciesId> m_shapes;
public:
	static AnimalSpeciesId create(AnimalSpeciesParamaters);
	[[nodiscard]] static ShapeId shapeForPercentGrown(AnimalSpeciesId id, Percent percentGrown);
	[[nodiscard]] static AnimalSpeciesId byName(std::string name);
	[[nodiscard]] static std::string getName(AnimalSpeciesId id);
	[[nodiscard]] static bool getSentient(AnimalSpeciesId id);
	[[nodiscard]] static std::array<AttributeLevel, 3> getStrength(AnimalSpeciesId id);
	[[nodiscard]] static std::array<AttributeLevel, 3> getDextarity(AnimalSpeciesId id);
	[[nodiscard]] static std::array<AttributeLevel, 3> getAgility(AnimalSpeciesId id);
	[[nodiscard]] static std::array<Mass, 3> getMass(AnimalSpeciesId id);
	[[nodiscard]] static std::array<Step, 2> getDeathAgeSteps(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsTillFullyGrown(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsTillDieWithoutFood(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsEatFrequency(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsTillDieWithoutFluid(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsFluidDrinkFrequency(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsTillDieInUnsafeTemperature(AnimalSpeciesId id);
	[[nodiscard]] static Temperature getMinimumSafeTemperature(AnimalSpeciesId id);
	[[nodiscard]] static Temperature getMaximumSafeTemperature(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsSleepFrequency(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsTillSleepOveride(AnimalSpeciesId id);
	[[nodiscard]] static Step getStepsSleepDuration(AnimalSpeciesId id);
	[[nodiscard]] static bool getNocturnal(AnimalSpeciesId id);
	[[nodiscard]] static bool getEatsMeat(AnimalSpeciesId id);
	[[nodiscard]] static bool getEatsLeaves(AnimalSpeciesId id);
	[[nodiscard]] static bool getEatsFruit(AnimalSpeciesId id);
	[[nodiscard]] static DistanceInBlocks getVisionRange(AnimalSpeciesId id);
	[[nodiscard]] static uint32_t getBodyScale(AnimalSpeciesId id);
	[[nodiscard]] static MaterialTypeId getMaterialType(AnimalSpeciesId id);
	[[nodiscard]] static MoveTypeId getMoveType(AnimalSpeciesId id);
	[[nodiscard]] static FluidTypeId getFluidType(AnimalSpeciesId id);
	[[nodiscard]] static BodyTypeId getBodyType(AnimalSpeciesId id);
	[[nodiscard]] static std::vector<ShapeId> getShapes(AnimalSpeciesId id);
	[[nodiscard]] static AnimalSpeciesId size();
};
inline AnimalSpecies animalSpeciesData;
