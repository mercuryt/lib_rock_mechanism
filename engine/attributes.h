#pragma once

#include "animalSpecies.h"
#include "config.h"
#include "types.h"

#include <sys/types.h>
#include <vector>
#include <array>

enum class AttributeType { Strength, Dextarity, Agility, Mass };

struct Attribute
{
	uint32_t speciesNewbornValue;
	uint32_t speciesAdultValue;
	uint32_t value;
	Percent baseModifierPercent;
	int32_t bonusOrPenalty;
	Attribute(uint32_t snv, uint32_t sav, uint32_t bmp, int32_t bop, Percent percentGrown) : speciesNewbornValue(snv), speciesAdultValue(sav), baseModifierPercent(bmp), bonusOrPenalty(bop) { setPercentGrown(percentGrown); }
	Attribute(const Json& data, uint32_t speciesNewbornValue, uint32_t speciesAdultValue, Percent percentGrown);
	void setPercentGrown(Percent percentGrown);
	Json toJson() const;
};
class Attributes
{
	Attribute strength; // 3.5
	Attribute dextarity; // 3.5
	Attribute agility;  // 3.5
	Attribute mass; // 3.5
	Mass unencomberedCarryMass; 
	Speed moveSpeed;
	uint32_t baseCombatScore;
	Percent percentGrown;
public:
	Attributes(const AnimalSpecies& species, std::array<uint32_t, 4> modifierPercents, std::array<int32_t, 4> bonusOrPenalties, Percent pg) :
		strength(species.strength[0], species.strength[1], modifierPercents[1], bonusOrPenalties[1], pg),
		dextarity(species.dextarity[0], species.dextarity[1], modifierPercents[1], bonusOrPenalties[1], pg),
		agility(species.agility[0], species.agility[1], modifierPercents[2], bonusOrPenalties[2], pg),
		mass(species.mass[0], species.mass[1], modifierPercents[3], bonusOrPenalties[3], pg),
		percentGrown(pg)
	{ generate(); }
	Attributes(const AnimalSpecies& species, Percent pg) :
		strength(species.strength[0], species.strength[1], 100, 0, pg),
		dextarity(species.dextarity[0], species.dextarity[1], 100, 0, pg),
		agility(species.agility[0], species.agility[1], 100, 0, pg),
		mass(species.mass[0], species.mass[1], 100, 0, pg),
		percentGrown(pg)
	{ generate(); }
	Attributes(const Json& data, const AnimalSpecies& species, Percent percentGrown);
	void updatePercentGrown(Percent percentGrown);
	void generate();
	void removeMass(Mass m)
	{
		assert(mass.value >= m);
		mass.value -= m;
	}
	uint32_t getStrength() const { return strength.value; }
	int32_t getStrengthBonusOrPenalty() const { return strength.bonusOrPenalty; }
	void setStrengthBonusOrPenalty(int32_t x) { strength.bonusOrPenalty = x; updateStrength(); generate(); }
	Percent getStrengthModifier() const { return strength.baseModifierPercent; }
	void setStrengthModifier(Percent x) { strength.baseModifierPercent = x; updateStrength(); generate(); }
	void updateStrength() { strength.setPercentGrown(percentGrown); }
	uint32_t getDextarity() const { return dextarity.value; }
	int32_t getDextarityBonusOrPenalty() const { return dextarity.bonusOrPenalty; }
	void setDextarityBonusOrPenalty(uint32_t x) { dextarity.bonusOrPenalty = x; updateDextarity(); generate(); }
	Percent getDextarityModifier() const { return dextarity.baseModifierPercent; }
	void setDextarityModifier(Percent x) { dextarity.baseModifierPercent = x; updateDextarity(); generate(); }
	void updateDextarity() { dextarity.setPercentGrown(percentGrown); }
	uint32_t getAgility() const { return agility.value; }
	int32_t getAgilityBonusOrPenalty() const { return agility.bonusOrPenalty; }
	void setAgilityBonusOrPenalty(uint32_t x) { agility.bonusOrPenalty = x; updateAgility(); generate(); }
	Percent getAgilityModifier() const { return agility.baseModifierPercent; }
	void setAgilityModifier(Percent x) { agility.baseModifierPercent = x; updateAgility(); generate(); }
	void updateAgility() { agility.setPercentGrown(percentGrown); }
	Mass getMass() const { return mass.value; }
	Mass getMassBonusOrPenalty() const { return mass.bonusOrPenalty; }
	void setMassBonusOrPenalty(uint32_t x) { mass.bonusOrPenalty = x; updateMass(); generate(); }
	Mass getMassModifier() const { return mass.baseModifierPercent; }
	void setMassModifier(Percent x) { mass.baseModifierPercent = x; updateMass(); generate(); }
	void updateMass() { mass.setPercentGrown(percentGrown); }
	Mass getUnencomberedCarryMass() const { return unencomberedCarryMass; }
	Speed getMoveSpeed() const { return moveSpeed; }
	uint32_t getBaseCombatScore() const { return baseCombatScore; }
	Json toJson() const;
};
