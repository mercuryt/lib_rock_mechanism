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
	AttributeLevel value;
	Percent baseModifierPercent;
	int32_t bonusOrPenalty;
	Attribute() = default; // default constructor needed to allow vector resize.
	Attribute(Percent bmp, int32_t bop, Percent percentGrown, const AnimalSpecies& species) : baseModifierPercent(bmp), bonusOrPenalty(bop) { setPercentGrown(percentGrown, species); }
	Attribute(const Json& data, Percent percentGrown, const AnimalSpecies& species);
	void setPercentGrown(Percent percentGrown, const AnimalSpecies& species);
	Json toJson() const;
};
class Attributes
{
	Attribute strength; // 3.5
	Attribute dextarity; // 3.5
	Attribute agility;  // 3.5
	// Hacking attribute to handle mass is a bit akward.
	Attribute mass; // 3.5
	Mass unencomberedCarryMass; 
	Speed moveSpeed;
	Stamina maxStamina;
	CombatScore baseCombatScore;
	Percent percentGrown;
public:
	Attributes(const AnimalSpecies& species, std::array<Percent, 4> modifierPercents, std::array<int32_t, 4> bonusOrPenalties, Percent pg) :
		strength(modifierPercents[1], bonusOrPenalties[1], pg, species),
		dextarity(modifierPercents[1], bonusOrPenalties[1], pg, species),
		agility(modifierPercents[2], bonusOrPenalties[2], pg, species),
		mass(modifierPercents[3], bonusOrPenalties[3], pg, species),
		percentGrown(pg)
	{ generate(); }
	Attributes(const AnimalSpecies& species, Percent pg) :
		strength(Percent::create(100), 0, pg, species),
		dextarity(Percent::create(100), 0, pg, species),
		agility(Percent::create(100), 0, pg, species),
		mass(Percent::create(100), 0, pg, species),
		percentGrown(pg)
	{ generate(); }
	Attributes(const Json& data, const AnimalSpecies& species, Percent percentGrown);
	void updatePercentGrown(Percent percentGrown);
	void generate();
	void removeMass(Mass m)
	{
		assert(mass.value.get() >= m.get());
		mass.value -= m.get();
	}
	[[nodiscard]] AttributeLevel getStrength() const { return strength.value; }
	[[nodiscard]] int32_t getStrengthBonusOrPenalty() const { return strength.bonusOrPenalty; }
	void setStrengthBonusOrPenalty(int32_t x, const AnimalSpecies& species) { strength.bonusOrPenalty = x; updateStrength(species); generate(); }
	[[nodiscard]] Percent getStrengthModifier() const { return strength.baseModifierPercent; }
	void setStrengthModifier(Percent x, const AnimalSpecies& species) { strength.baseModifierPercent = x; updateStrength(species); generate(); }
	void updateStrength(const AnimalSpecies& species) { strength.setPercentGrown(percentGrown, species); }
	[[nodiscard]] AttributeLevel getDextarity() const { return dextarity.value; }
	[[nodiscard]] int32_t getDextarityBonusOrPenalty() const { return dextarity.bonusOrPenalty; }
	void setDextarityBonusOrPenalty(int32_t x, const AnimalSpecies& species) { dextarity.bonusOrPenalty = x; updateDextarity(species); generate(); }
	[[nodiscard]] Percent getDextarityModifier() const { return dextarity.baseModifierPercent; }
	void setDextarityModifier(Percent x, const AnimalSpecies& species) { dextarity.baseModifierPercent = x; updateDextarity(species); generate(); }
	void updateDextarity(const AnimalSpecies& species) { dextarity.setPercentGrown(percentGrown, species); }
	[[nodiscard]] AttributeLevel getAgility() const { return agility.value; }
	[[nodiscard]] int32_t getAgilityBonusOrPenalty() const { return agility.bonusOrPenalty; }
	void setAgilityBonusOrPenalty(int32_t x, const AnimalSpecies& species) { agility.bonusOrPenalty = x; updateAgility(species); generate(); }
	[[nodiscard]] Percent getAgilityModifier() const { return agility.baseModifierPercent; }
	void setAgilityModifier(Percent x, const AnimalSpecies& species) { agility.baseModifierPercent = x; updateAgility(species); generate(); }
	void updateAgility(const AnimalSpecies& species) { agility.setPercentGrown(percentGrown, species); }
	[[nodiscard]] Mass getMass() const { return Mass::create(mass.value.get()); }
	[[nodiscard]] int32_t getMassBonusOrPenalty() const { return mass.bonusOrPenalty; }
	void setMassBonusOrPenalty(int32_t x, const AnimalSpecies& species) { mass.bonusOrPenalty = x; updateMass(species); generate(); }
	[[nodiscard]] Percent getMassModifier() const { return mass.baseModifierPercent; }
	void setMassModifier(Percent x, const AnimalSpecies& species) { mass.baseModifierPercent = x; updateMass(species); generate(); }
	void updateMass(const AnimalSpecies& species) { mass.setPercentGrown(percentGrown, species); }
	[[nodiscard]] Mass getUnencomberedCarryMass() const { return unencomberedCarryMass; }
	[[nodiscard]] Speed getMoveSpeed() const { return moveSpeed; }
	[[nodiscard]] CombatScore getBaseCombatScore() const { return baseCombatScore; }
	[[nodiscard]] Json toJson() const;
};
