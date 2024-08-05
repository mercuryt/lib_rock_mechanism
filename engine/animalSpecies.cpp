#include "animalSpecies.h"
const Shape& AnimalSpecies::shapeForPercentGrown(AnimalSpeciesId id, Percent percentGrown)
{
	uint index = util::scaleByPercentRange(0, data.m_shapes.at(id).size() - 1, percentGrown);
	return *data.m_shapes.at(id).at(index);
}
// Static method.
AnimalSpeciesId AnimalSpecies::byName(std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return AnimalSpeciesId::create(found - data.m_name.begin());
}
AnimalSpeciesId AnimalSpecies::create(AnimalSpeciesParamaters p)
{
	data.m_name.add(p.name);
	data.m_sentient.add(p.sentient);
	data.m_strength.add(p.strength);
	data.m_dextarity.add(p.dextarity);
	data.m_agility.add(p.agility);
	data.m_mass.add(p.mass);
	data.m_deathAgeSteps.add(p.deathAgeSteps);
	data.m_stepsTillFullyGrown.add(p.stepsTillFullyGrown);
	data.m_stepsTillDieWithoutFood.add(p.stepsTillDieWithoutFood);
	data.m_stepsEatFrequency.add(p.stepsEatFrequency);
	data.m_stepsTillDieWithoutFluid.add(p.stepsTillDieWithoutFluid);
	data.m_stepsFluidDrinkFreqency.add(p.stepsFluidDrinkFreqency);
	data.m_stepsTillDieInUnsafeTemperature.add(p.stepsTillDieInUnsafeTemperature);
	data.m_minimumSafeTemperature.add(p.minimumSafeTemperature);
	data.m_maximumSafeTemperature.add(p.maximumSafeTemperature);
	data.m_stepsSleepFrequency.add(p.stepsSleepFrequency);
	data.m_stepsTillSleepOveride.add(p.stepsTillSleepOveride);
	data.m_stepsSleepDuration.add(p.stepsSleepDuration);
	data.m_nocturnal.add(p.nocturnal);
	data.m_eatsMeat.add(p.eatsMeat);
	data.m_eatsLeaves.add(p.eatsLeaves);
	data.m_eatsFruit.add(p.eatsFruit);
	data.m_visionRange.add(p.visionRange);
	data.m_bodyScale.add(p.bodyScale);
	data.m_materialType.add(p.materialType);
	data.m_moveType.add(p.moveType);
	data.m_fluidType.add(p.fluidType);
	data.m_bodyType.add(p.bodyType);
	data.m_shapes.add(p.shapes);
	return AnimalSpeciesId::create(data.m_name.size());
}
