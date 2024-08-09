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
	static AnimalSpecies data;
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
	[[nodiscard]] static std::string getName(AnimalSpeciesId id) { return data.m_name.at(id); };
	[[nodiscard]] static bool getSentient(AnimalSpeciesId id) { return data.m_sentient.at(id); };
	[[nodiscard]] static std::array<AttributeLevel, 3> getStrength(AnimalSpeciesId id) { return data.m_strength.at(id); };
	[[nodiscard]] static std::array<AttributeLevel, 3> getDextarity(AnimalSpeciesId id) { return data.m_dextarity.at(id); };
	[[nodiscard]] static std::array<AttributeLevel, 3> getAgility(AnimalSpeciesId id) { return data.m_agility.at(id); };
	[[nodiscard]] static std::array<Mass, 3> getMass(AnimalSpeciesId id) { return data.m_mass.at(id); };
	[[nodiscard]] static std::array<Step, 2> getDeathAgeSteps(AnimalSpeciesId id) { return data.m_deathAgeSteps.at(id); };
	[[nodiscard]] static Step getStepsTillFullyGrown(AnimalSpeciesId id) { return data.m_stepsTillFullyGrown.at(id); };
	[[nodiscard]] static Step getStepsTillDieWithoutFood(AnimalSpeciesId id) { return data.m_stepsTillDieWithoutFood.at(id); };
	[[nodiscard]] static Step getStepsEatFrequency(AnimalSpeciesId id) { return data.m_stepsEatFrequency.at(id); };
	[[nodiscard]] static Step getStepsTillDieWithoutFluid(AnimalSpeciesId id) { return data.m_stepsTillDieWithoutFluid.at(id); };
	[[nodiscard]] static Step getStepsFluidDrinkFrequency(AnimalSpeciesId id) { return data.m_stepsFluidDrinkFrequency.at(id); };
	[[nodiscard]] static Step getStepsTillDieInUnsafeTemperature(AnimalSpeciesId id) { return data.m_stepsTillDieInUnsafeTemperature.at(id); };
	[[nodiscard]] static Temperature getMinimumSafeTemperature(AnimalSpeciesId id) { return data.m_minimumSafeTemperature.at(id); };
	[[nodiscard]] static Temperature getMaximumSafeTemperature(AnimalSpeciesId id) { return data.m_maximumSafeTemperature.at(id); };
	[[nodiscard]] static Step getStepsSleepFrequency(AnimalSpeciesId id) { return data.m_stepsSleepFrequency.at(id); };
	[[nodiscard]] static Step getStepsTillSleepOveride(AnimalSpeciesId id) { return data.m_stepsTillSleepOveride.at(id); };
	[[nodiscard]] static Step getStepsSleepDuration(AnimalSpeciesId id) { return data.m_stepsSleepDuration.at(id); };
	[[nodiscard]] static bool getNocturnal(AnimalSpeciesId id) { return data.m_nocturnal.at(id); };
	[[nodiscard]] static bool getEatsMeat(AnimalSpeciesId id) { return data.m_eatsMeat.at(id); };
	[[nodiscard]] static bool getEatsLeaves(AnimalSpeciesId id) { return data.m_eatsLeaves.at(id); };
	[[nodiscard]] static bool getEatsFruit(AnimalSpeciesId id) { return data.m_eatsFruit.at(id); };
	[[nodiscard]] static DistanceInBlocks getVisionRange(AnimalSpeciesId id) { return data.m_visionRange.at(id); };
	[[nodiscard]] static uint32_t getBodyScale(AnimalSpeciesId id) { return data.m_bodyScale.at(id); };
	[[nodiscard]] static MaterialTypeId getMaterialType(AnimalSpeciesId id) { return data.m_materialType.at(id); };
	[[nodiscard]] static MoveTypeId getMoveType(AnimalSpeciesId id) { return data.m_moveType.at(id); };
	[[nodiscard]] static FluidTypeId getFluidType(AnimalSpeciesId id) { return data.m_fluidType.at(id); };
	[[nodiscard]] static BodyTypeId getBodyType(AnimalSpeciesId id) { return data.m_bodyType.at(id); };
	[[nodiscard]] static std::vector<ShapeId> getShapes(AnimalSpeciesId id) { return data.m_shapes.at(id); };
	[[nodiscard]] static AnimalSpeciesId size() { return AnimalSpeciesId::create(data.m_name.size()); }
};
