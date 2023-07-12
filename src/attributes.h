#pragma once

#include "animalSpecies.h"
#include "util.h"
#include "config.h"

#include <vector>
#include <array>

enum class AttributeType { Strength, Dextarity, Agility, Mass };

struct Attribute
{
	uint32_t value;
	const uint32_t& speciesNewbornValue;
	const uint32_t& speciesAdultValue;
	uint32_t baseModifierPercent;
	int32_t bonusOrPenalty;
	Attribute(const uint32_t& snv, const uint32_t& sav, uint32_t bmp, int32_t bop, uint32_t percentGrown) : speciesNewbornValue(snv), speciesAdultValue(sav), baseModifierPercent(bmp), bonusOrPenalty(bop) { setPercentGrown(percentGrown); }
	void setPercentGrown(uint32_t percentGrown)
	{
		value = util::scaleByPercent(
				util::scaleByPercentRange(percentGrown, speciesNewbornValue, speciesAdultValue),
				baseModifierPercent) + bonusOrPenalty;
	}
};
class Attributes
{
	Attribute strength;
	Attribute dextarity;
	Attribute agility;
	Attribute mass;
	uint32_t unencomberedCarryMass;
	uint32_t moveSpeed;
public:
	Attributes(AnimalSpecies& species, std::array<uint32_t, 4> modifierPercents, std::array<int32_t, 4> bonusOrPenalties, uint32_t percentGrown) :
		strength(species.strength[0], species.strength[1], modifierPercents[1], bonusOrPenalties[1], percentGrown),
		dextarity(species.dextarity[0], species.dextarity[1], modifierPercents[1], bonusOrPenalties[1], percentGrown),
		agility(species.agility[0], species.agility[1], modifierPercents[2], bonusOrPenalties[2], percentGrown),
		mass(species.mass[0], species.mass[1], modifierPercents[3], bonusOrPenalties[3], percentGrown) { }
	void updatePercentGrown(uint32_t percentGrown)
	{
		strength.setPercentGrown(percentGrown);
		dextarity.setPercentGrown(percentGrown);
		agility.setPercentGrown(percentGrown);
		mass.setPercentGrown(percentGrown);
		generate();
	}
	void generate()
	{
		unencomberedCarryMass = strength.value * Config::unitsOfCarryMassPerUnitOfStrength;
		moveSpeed = agility.value * Config::unitsOfMoveSpeedPerUnitOfAgility;
	}
	uint32_t getStrength(){ return strength.value; }
	void addStrengthBonusOrPenalty(int32_t x) { strength.bonusOrPenalty =+ x; generate(); }
	uint32_t getDextarity(){ return dextarity.value; }
	void addDextarityBonusOrPenalty(uint32_t x) { dextarity.bonusOrPenalty =+ x; generate(); }
	uint32_t getAgility(){ return agility.value; }
	void addAgilityBonusOrPenalty(uint32_t x) { agility.bonusOrPenalty =+ x; generate(); }
	uint32_t getMass(){ return mass.value; }
	void addMassBonusOrPenalty(uint32_t x) { mass.bonusOrPenalty =+ x; generate(); }
	uint32_t getUnencomberedCarryMass(){ return unencomberedCarryMass; }
	uint32_t getMoveSpeed(){ return moveSpeed; }
};
