#pragma once
#include "types.h"
#include "body.h"
#include <string>
struct AnimalSpeciesParamaters
{
	std::string name;
	bool  sentient;
	// Min, max, newborn.
	std::array<AttributeLevel, 3>  strength;
	std::array<AttributeLevel, 3>  dextarity;
	std::array<AttributeLevel, 3>  agility;
	std::array<Mass, 3>  mass;
	std::array<Step, 2>  deathAgeSteps;
	Step  stepsTillFullyGrown;
	Step  stepsTillDieWithoutFood;
	Step  stepsEatFrequency;
	Step  stepsTillDieWithoutFluid;
	Step  stepsFluidDrinkFreqency;
	Step  stepsTillDieInUnsafeTemperature;
	Temperature  minimumSafeTemperature;
	Temperature  maximumSafeTemperature;
	Step  stepsSleepFrequency;
	Step  stepsTillSleepOveride;
	Step  stepsSleepDuration;
	bool  nocturnal;
	bool  eatsMeat;
	bool  eatsLeaves;
	bool  eatsFruit;
	DistanceInBlocks  visionRange;
	uint32_t  bodyScale;
	MaterialTypeId  materialType;
	MoveTypeId  moveType;
	FluidTypeId  fluidType;
	BodyTypeId  bodyType;
	std::vector<Shape*>  shapes;
};
class AnimalSpecies
{
	static AnimalSpecies data;
	DataVector<std::string , AnimalSpeciesId> m_name;
	DataVector<bool , AnimalSpeciesId> m_sentient;
	// Min, max, newborn.
	DataVector<std::array<AttributeLevel, 3> , AnimalSpeciesId> m_strength;
	DataVector<std::array<AttributeLevel, 3> , AnimalSpeciesId> m_dextarity;
	DataVector<std::array<AttributeLevel, 3> , AnimalSpeciesId> m_agility;
	DataVector<std::array<Mass, 3> , AnimalSpeciesId> m_mass;
	DataVector<std::array<Step, 2> , AnimalSpeciesId> m_deathAgeSteps;
	DataVector<Step , AnimalSpeciesId> m_stepsTillFullyGrown;
	DataVector<Step , AnimalSpeciesId> m_stepsTillDieWithoutFood;
	DataVector<Step , AnimalSpeciesId> m_stepsEatFrequency;
	DataVector<Step , AnimalSpeciesId> m_stepsTillDieWithoutFluid;
	DataVector<Step , AnimalSpeciesId> m_stepsFluidDrinkFreqency;
	DataVector<Step , AnimalSpeciesId> m_stepsTillDieInUnsafeTemperature;
	DataVector<Temperature , AnimalSpeciesId> m_minimumSafeTemperature;
	DataVector<Temperature , AnimalSpeciesId> m_maximumSafeTemperature;
	DataVector<Step , AnimalSpeciesId> m_stepsSleepFrequency;
	DataVector<Step , AnimalSpeciesId> m_stepsTillSleepOveride;
	DataVector<Step , AnimalSpeciesId> m_stepsSleepDuration;
	DataVector<bool , AnimalSpeciesId> m_nocturnal;
	DataVector<bool , AnimalSpeciesId> m_eatsMeat;
	DataVector<bool , AnimalSpeciesId> m_eatsLeaves;
	DataVector<bool , AnimalSpeciesId> m_eatsFruit;
	DataVector<DistanceInBlocks , AnimalSpeciesId> m_visionRange;
	DataVector<uint32_t , AnimalSpeciesId> m_bodyScale;
	DataVector<MaterialTypeId , AnimalSpeciesId> m_materialType;
	DataVector<MoveTypeId , AnimalSpeciesId> m_moveType;
	DataVector<FluidTypeId , AnimalSpeciesId> m_fluidType;
	DataVector<BodyTypeId , AnimalSpeciesId> m_bodyType;
	DataVector<std::vector<Shape*> , AnimalSpeciesId> m_shapes;
	static const Shape& shapeForPercentGrown(AnimalSpeciesId id, Percent percentGrown);
	static AnimalSpeciesId byName(std::string name);
	static AnimalSpeciesId create(AnimalSpeciesParamaters);
};
