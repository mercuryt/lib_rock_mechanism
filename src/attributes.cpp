#include "attributes.h"
#include "util.h"
void Attribute::setPercentGrown(uint32_t percentGrown)
{
	value = util::scaleByPercent(
			util::scaleByPercentRange(percentGrown, speciesNewbornValue, speciesAdultValue),
			baseModifierPercent) + bonusOrPenalty;
}
void Attributes::updatePercentGrown(uint32_t percentGrown)
{
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
}
