#include "attributes.h"
#include "animalSpecies.h"
#include "types.h"
#include "util.h"
void Attribute::setPercentGrown(Percent percentGrown, const AnimalSpecies& species)
{
	value = AttributeLevel::create(util::scaleByPercentRange(speciesNewbornValue.get(), speciesAdultValue.get(), percentGrown) + bonusOrPenalty);
}
Json Attribute::toJson() const 
{
	Json data;
	data["value"] = value;
	data["baseModifierPercent"] = baseModifierPercent;
	data["bonusOrPenalty"] = bonusOrPenalty;
	return data;
}
Attribute::Attribute(const Json& data, Percent percentGrown, const AnimalSpecies& species) :
	value(data["value"].get<AttributeLevel>()),
	baseModifierPercent(data["baseModifierPercent"].get<Percent>()),
	bonusOrPenalty(data["bonusOrPenalty"].get<int32_t>())
{ setPercentGrown(percentGrown, species); }
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
	unencomberedCarryMass = Mass::create(strength.value.get() * Config::unitsOfCarryMassPerUnitOfStrength);
	moveSpeed = Speed::create(agility.value.get() * Config::unitsOfMoveSpeedPerUnitOfAgility);
	baseCombatScore = CombatScore::create(
		(strength.value.get() * Config::pointsOfCombatScorePerUnitOfStrength) +
		(agility.value.get() * Config::pointsOfCombatScorePerUnitOfAgility) +
		(dextarity.value.get() * Config::pointsOfCombatScorePerUnitOfDextarity)
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
Attributes::Attributes(const Json& data, const AnimalSpecies& species, Percent pg) :
	strength(data["strength"], species.strength[0], species.strength[1], pg),
	dextarity(data["dextarity"], species.dextarity[0], species.dextarity[1], pg),
	agility(data["agility"], species.agility[0], species.agility[1], pg),
	mass(data["mass"], AttributeLevel::create(species.mass[0].get()), AttributeLevel::create(species.mass[1].get()), pg),
	percentGrown(pg) { generate(); }
