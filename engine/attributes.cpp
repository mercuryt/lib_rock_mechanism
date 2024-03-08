#include "attributes.h"
#include "animalSpecies.h"
#include "types.h"
#include "util.h"
void Attribute::setPercentGrown(Percent percentGrown)
{
	Percent growthPercent = util::scaleByPercentRange(speciesNewbornValue, speciesAdultValue, percentGrown);
	value = util::scaleByPercent(baseModifierPercent, growthPercent) + bonusOrPenalty;
}
Json Attribute::toJson() const 
{
	Json data;
	data["value"] = value;
	data["baseModifierPercent"] = baseModifierPercent;
	data["bonusOrPenalty"] = bonusOrPenalty;
	return data;
}
Attribute::Attribute(const Json& data, const uint32_t& speciesNewbornValue, const uint32_t& speciesAdultValue, Percent percentGrown) :
	value(data["value"].get<uint32_t>()),
	speciesNewbornValue(speciesNewbornValue),
	speciesAdultValue(speciesAdultValue),
	baseModifierPercent(data["baseModifierPercent"].get<Percent>()),
	bonusOrPenalty(data["bonusOrPenalty"].get<int32_t>())
{ setPercentGrown(percentGrown); }
void Attributes::updatePercentGrown(Percent pg)
{
	percentGrown = pg;
	strength.setPercentGrown(percentGrown);
	dextarity.setPercentGrown(percentGrown);
	agility.setPercentGrown(percentGrown);
	mass.setPercentGrown(percentGrown);
	generate();
}
void Attributes::generate()
{
	unencomberedCarryMass = strength.value * Config::unitsOfCarryMassPerUnitOfStrength;
	moveSpeed = agility.value * Config::unitsOfMoveSpeedPerUnitOfAgility;
	baseCombatScore = (
		(strength.value * Config::pointsOfCombatScorePerUnitOfStrength) +
		(agility.value * Config::pointsOfCombatScorePerUnitOfAgility) +
		(dextarity.value * Config::pointsOfCombatScorePerUnitOfDextarity)
	);
}
Json Attributes::toJson() const
{
	Json data;
	data["strength"] = strength.toJson();
	data["dextarity"] = dextarity.toJson();
	data["agility"] = agility.toJson();
	data["mass"] = mass.toJson();
	return data;
}
Attributes::Attributes(const Json& data, const AnimalSpecies& species, Percent percentGrown) :
	strength(data["strength"], species.strength[0], species.strength[1], percentGrown),
	dextarity(data["dextarity"], species.dextarity[0], species.dextarity[1], percentGrown),
	agility(data["agility"], species.agility[0], species.agility[1], percentGrown),
	mass(data["mass"], species.mass[0], species.mass[1], percentGrown) { generate(); }
