#include "actors.h"
#include "animalSpecies.h"
#include "config.h"
#include "types.h"

void Actors::attributes_onUpdateGrowthPercent(ActorIndex index)
{
	updateStrength(index);
	updateAgility(index);
	updateDextarity(index);
	updateMass(index);
	canPickUp_updateUnencomberedCarryMass(index);
	combat_update(index);
	move_updateIndividualSpeed(index);
}

[[nodiscard]] AttributeLevel Actors::getStrength(ActorIndex index) const { return m_strength.at(index); }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getStrengthBonusOrPenalty(ActorIndex index) const { return m_strengthBonusOrPenalty.at(index); }
[[nodiscard]] float Actors::getStrengthModifier(ActorIndex index) const { return m_strengthModifier.at(index); }
void Actors::addStrengthModifier(ActorIndex index, float modifier) 
{ 
	m_strengthModifier.at(index) += modifier; 
	onStrengthChanged(index);
}
void Actors::addStrengthBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty)
{
	m_strengthBonusOrPenalty.at(index) += bonusOrPenalty;
	onStrengthChanged(index);
}
void Actors::onStrengthChanged(ActorIndex index)
{
	updateStrength(index);
	canPickUp_updateUnencomberedCarryMass(index);
	combat_update(index);
}
void Actors::updateStrength(ActorIndex index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	AttributeLevel max = AnimalSpecies::getStrength(species)[0];
	AttributeLevel min = AnimalSpecies::getStrength(species)[1];
	m_strength.at(index) = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] AttributeLevel Actors::getDextarity(ActorIndex index) const { return m_dextarity.at(index); }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getDextarityBonusOrPenalty(ActorIndex index) const { return m_dextarityBonusOrPenalty.at(index); }
[[nodiscard]] float Actors::getDextarityModifier(ActorIndex index) const { return m_dextarityModifier.at(index); }
void Actors::addDextarityModifier(ActorIndex index, float modifier) 
{ 
	m_dextarityModifier.at(index) += modifier; 
	onDextarityChanged(index);
}
void Actors::addDextarityBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty)
{
	m_dextarityBonusOrPenalty.at(index) += bonusOrPenalty;
	onDextarityChanged(index);
}
void Actors::onDextarityChanged(ActorIndex index)
{
	updateDextarity(index);
	combat_update(index);
}
void Actors::updateDextarity(ActorIndex index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	AttributeLevel max = AnimalSpecies::getDextarity(species)[0];
	AttributeLevel min = AnimalSpecies::getDextarity(species)[1];
	m_dextarity.at(index) = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] AttributeLevel Actors::getAgility(ActorIndex index) const { return m_agility.at(index); }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getAgilityBonusOrPenalty(ActorIndex index) const { return m_agilityBonusOrPenalty.at(index); }
[[nodiscard]] float Actors::getAgilityModifier(ActorIndex index) const { return m_agilityModifier.at(index); }
void Actors::addAgilityModifier(ActorIndex index, float modifier) 
{ 
	m_agilityModifier.at(index) += modifier; 
	onAgilityChanged(index);
}
void Actors::addAgilityBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty)
{
	m_agilityBonusOrPenalty.at(index) += bonusOrPenalty;
	onAgilityChanged(index);
}
void Actors::onAgilityChanged(ActorIndex index)
{
	updateAgility(index);
	move_updateIndividualSpeed(index);
	combat_update(index);
}
void Actors::updateAgility(ActorIndex index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	AttributeLevel max = AnimalSpecies::getAgility(species)[0];
	AttributeLevel min = AnimalSpecies::getAgility(species)[1];
	m_agility.at(index) = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] Mass Actors::getMass(ActorIndex index) const { return m_mass.at(index); }
[[nodiscard]] int32_t Actors::getMassBonusOrPenalty(ActorIndex index) const { return m_massBonusOrPenalty.at(index); }
[[nodiscard]] float Actors::getMassModifier(ActorIndex index) const { return m_massModifier.at(index); }
void Actors::addMassModifier(ActorIndex index, float modifier) 
{ 
	m_massModifier.at(index) += modifier; 
	onMassChanged(index);
}
void Actors::addMassBonusOrPenalty(ActorIndex index, uint32_t bonusOrPenalty)
{
	m_massBonusOrPenalty.at(index) += bonusOrPenalty;
	onMassChanged(index);
}
void Actors::updateMass(ActorIndex index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	Mass max = AnimalSpecies::getMass(species)[0];
	Mass min = AnimalSpecies::getMass(species)[1];
	m_mass.at(index) = Mass::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}
void Actors::onMassChanged(ActorIndex index)
{
	updateMass(index);
	move_updateIndividualSpeed(index);
	//TODO: mass does not currently affect combat?
	//combat_update(index);
}
