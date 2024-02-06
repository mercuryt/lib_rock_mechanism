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
	uint32_t value;
	const uint32_t& speciesNewbornValue;
	const uint32_t& speciesAdultValue;
	Percent baseModifierPercent;
	int32_t bonusOrPenalty;
	Attribute(const uint32_t& snv, const uint32_t& sav, uint32_t bmp, int32_t bop, Percent percentGrown) : speciesNewbornValue(snv), speciesAdultValue(sav), baseModifierPercent(bmp), bonusOrPenalty(bop) { setPercentGrown(percentGrown); }
	Attribute(const Json& data, const uint32_t& speciesNewbornValue, const uint32_t& speciesAdultValue, Percent percentGrown);
	void setPercentGrown(Percent percentGrown);
	Json toJson() const;
};
class Attributes
{
	Attribute strength;
	Attribute dextarity;
	Attribute agility;
	Attribute mass;
	Mass unencomberedCarryMass;
	uint32_t moveSpeed;
	uint32_t baseCombatScore;
public:
	Attributes(const AnimalSpecies& species, std::array<uint32_t, 4> modifierPercents, std::array<int32_t, 4> bonusOrPenalties, Percent percentGrown) :
		strength(species.strength[0], species.strength[1], modifierPercents[1], bonusOrPenalties[1], percentGrown),
		dextarity(species.dextarity[0], species.dextarity[1], modifierPercents[1], bonusOrPenalties[1], percentGrown),
		agility(species.agility[0], species.agility[1], modifierPercents[2], bonusOrPenalties[2], percentGrown),
		mass(species.mass[0], species.mass[1], modifierPercents[3], bonusOrPenalties[3], percentGrown) 
	{ generate(); }
	Attributes(const AnimalSpecies& species, Percent percentGrown) :
		strength(species.strength[0], species.strength[1], 100, 0, percentGrown),
		dextarity(species.dextarity[0], species.dextarity[1], 100, 0, percentGrown),
		agility(species.agility[0], species.agility[1], 100, 0, percentGrown),
		mass(species.mass[0], species.mass[1], 100, 0, percentGrown)
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
	void addStrengthBonusOrPenalty(int32_t x) { strength.bonusOrPenalty += x; generate(); }
	void setStrengthBonusOrPenalty(int32_t x) { strength.bonusOrPenalty = x; generate(); }
	uint32_t getDextarity() const { return dextarity.value; }
	void addDextarityBonusOrPenalty(uint32_t x) { dextarity.bonusOrPenalty += x; generate(); }
	void setDextarityBonusOrPenalty(uint32_t x) { dextarity.bonusOrPenalty = x; generate(); }
	uint32_t getAgility() const { return agility.value; }
	void addAgilityBonusOrPenalty(uint32_t x) { agility.bonusOrPenalty += x; generate(); }
	void setAgilityBonusOrPenalty(uint32_t x) { agility.bonusOrPenalty = x; generate(); }
	Mass getMass() const { return mass.value; }
	void addMassBonusOrPenalty(uint32_t x) { mass.bonusOrPenalty += x; generate(); }
	void setMassBonusOrPenalty(uint32_t x) { mass.bonusOrPenalty = x; generate(); }
	uint32_t getUnencomberedCarryMass() const { return unencomberedCarryMass; }
	uint32_t getMoveSpeed() const { return moveSpeed; }
	uint32_t getBaseCombatScore() const { return baseCombatScore; }
	Json toJson() const;
};
