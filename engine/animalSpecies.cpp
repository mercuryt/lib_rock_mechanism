#include "animalSpecies.h"
ShapeId AnimalSpecies::shapeForPercentGrown(AnimalSpeciesId id, Percent percentGrown)
{
	uint index = util::scaleByPercentRange(0, animalSpeciesData.m_shapes[id].size() - 1, percentGrown);
	return animalSpeciesData.m_shapes[id][index];
}
// Static method.
AnimalSpeciesId AnimalSpecies::byName(std::string name)
{
	auto found = animalSpeciesData.m_name.find(name);
	assert(found != animalSpeciesData.m_name.end());
	return AnimalSpeciesId::create(found - animalSpeciesData.m_name.begin());
}
AnimalSpeciesId AnimalSpecies::create(AnimalSpeciesParamaters p)
{
	animalSpeciesData.m_name.add(p.name);
	animalSpeciesData.m_sentient.add(p.sentient);
	animalSpeciesData.m_strength.add(p.strength);
	animalSpeciesData.m_dextarity.add(p.dextarity);
	animalSpeciesData.m_agility.add(p.agility);
	animalSpeciesData.m_mass.add(p.mass);
	animalSpeciesData.m_deathAgeSteps.add(p.deathAgeSteps);
	animalSpeciesData.m_stepsTillFullyGrown.add(p.stepsTillFullyGrown);
	animalSpeciesData.m_stepsTillDieWithoutFood.add(p.stepsTillDieWithoutFood);
	animalSpeciesData.m_stepsEatFrequency.add(p.stepsEatFrequency);
	animalSpeciesData.m_stepsTillDieWithoutFluid.add(p.stepsTillDieWithoutFluid);
	animalSpeciesData.m_stepsFluidDrinkFrequency.add(p.stepsFluidDrinkFrequency);
	animalSpeciesData.m_stepsTillDieInUnsafeTemperature.add(p.stepsTillDieInUnsafeTemperature);
	animalSpeciesData.m_minimumSafeTemperature.add(p.minimumSafeTemperature);
	animalSpeciesData.m_maximumSafeTemperature.add(p.maximumSafeTemperature);
	animalSpeciesData.m_stepsSleepFrequency.add(p.stepsSleepFrequency);
	animalSpeciesData.m_stepsTillSleepOveride.add(p.stepsTillSleepOveride);
	animalSpeciesData.m_stepsSleepDuration.add(p.stepsSleepDuration);
	animalSpeciesData.m_nocturnal.add(p.nocturnal);
	animalSpeciesData.m_eatsMeat.add(p.eatsMeat);
	animalSpeciesData.m_eatsLeaves.add(p.eatsLeaves);
	animalSpeciesData.m_eatsFruit.add(p.eatsFruit);
	animalSpeciesData.m_visionRange.add(p.visionRange);
	animalSpeciesData.m_bodyScale.add(p.bodyScale);
	animalSpeciesData.m_materialType.add(p.materialType);
	animalSpeciesData.m_moveType.add(p.moveType);
	animalSpeciesData.m_fluidType.add(p.fluidType);
	animalSpeciesData.m_bodyType.add(p.bodyType);
	animalSpeciesData.m_shapes.add(p.shapes);
	return AnimalSpeciesId::create(animalSpeciesData.m_name.size());
}
std::string AnimalSpecies::getName(AnimalSpeciesId id) { return animalSpeciesData.m_name[id]; };
bool AnimalSpecies::getSentient(AnimalSpeciesId id) { return animalSpeciesData.m_sentient[id]; };
std::array<AttributeLevel, 3> AnimalSpecies::getStrength(AnimalSpeciesId id) { return animalSpeciesData.m_strength[id]; };
std::array<AttributeLevel, 3> AnimalSpecies::getDextarity(AnimalSpeciesId id) { return animalSpeciesData.m_dextarity[id]; };
std::array<AttributeLevel, 3> AnimalSpecies::getAgility(AnimalSpeciesId id) { return animalSpeciesData.m_agility[id]; };
std::array<Mass, 3> AnimalSpecies::getMass(AnimalSpeciesId id) { return animalSpeciesData.m_mass[id]; };
std::array<Step, 2> AnimalSpecies::getDeathAgeSteps(AnimalSpeciesId id) { return animalSpeciesData.m_deathAgeSteps[id]; };
Step AnimalSpecies::getStepsTillFullyGrown(AnimalSpeciesId id) { return animalSpeciesData.m_stepsTillFullyGrown[id]; };
Step AnimalSpecies::getStepsTillDieWithoutFood(AnimalSpeciesId id) { return animalSpeciesData.m_stepsTillDieWithoutFood[id]; };
Step AnimalSpecies::getStepsEatFrequency(AnimalSpeciesId id) { return animalSpeciesData.m_stepsEatFrequency[id]; };
Step AnimalSpecies::getStepsTillDieWithoutFluid(AnimalSpeciesId id) { return animalSpeciesData.m_stepsTillDieWithoutFluid[id]; };
Step AnimalSpecies::getStepsFluidDrinkFrequency(AnimalSpeciesId id) { return animalSpeciesData.m_stepsFluidDrinkFrequency[id]; };
Step AnimalSpecies::getStepsTillDieInUnsafeTemperature(AnimalSpeciesId id) { return animalSpeciesData.m_stepsTillDieInUnsafeTemperature[id]; };
Temperature AnimalSpecies::getMinimumSafeTemperature(AnimalSpeciesId id) { return animalSpeciesData.m_minimumSafeTemperature[id]; };
Temperature AnimalSpecies::getMaximumSafeTemperature(AnimalSpeciesId id) { return animalSpeciesData.m_maximumSafeTemperature[id]; };
Step AnimalSpecies::getStepsSleepFrequency(AnimalSpeciesId id) { return animalSpeciesData.m_stepsSleepFrequency[id]; };
Step AnimalSpecies::getStepsTillSleepOveride(AnimalSpeciesId id) { return animalSpeciesData.m_stepsTillSleepOveride[id]; };
Step AnimalSpecies::getStepsSleepDuration(AnimalSpeciesId id) { return animalSpeciesData.m_stepsSleepDuration[id]; };
bool AnimalSpecies::getNocturnal(AnimalSpeciesId id) { return animalSpeciesData.m_nocturnal[id]; };
bool AnimalSpecies::getEatsMeat(AnimalSpeciesId id) { return animalSpeciesData.m_eatsMeat[id]; };
bool AnimalSpecies::getEatsLeaves(AnimalSpeciesId id) { return animalSpeciesData.m_eatsLeaves[id]; };
bool AnimalSpecies::getEatsFruit(AnimalSpeciesId id) { return animalSpeciesData.m_eatsFruit[id]; };
DistanceInBlocks AnimalSpecies::getVisionRange(AnimalSpeciesId id) { return animalSpeciesData.m_visionRange[id]; };
uint32_t AnimalSpecies::getBodyScale(AnimalSpeciesId id) { return animalSpeciesData.m_bodyScale[id]; };
MaterialTypeId AnimalSpecies::getMaterialType(AnimalSpeciesId id) { return animalSpeciesData.m_materialType[id]; };
MoveTypeId AnimalSpecies::getMoveType(AnimalSpeciesId id) { return animalSpeciesData.m_moveType[id]; };
FluidTypeId AnimalSpecies::getFluidType(AnimalSpeciesId id) { return animalSpeciesData.m_fluidType[id]; };
BodyTypeId AnimalSpecies::getBodyType(AnimalSpeciesId id) { return animalSpeciesData.m_bodyType[id]; };
std::vector<ShapeId> AnimalSpecies::getShapes(AnimalSpeciesId id) { return animalSpeciesData.m_shapes[id]; };
AnimalSpeciesId AnimalSpecies::size() { return AnimalSpeciesId::create(animalSpeciesData.m_name.size()); }
