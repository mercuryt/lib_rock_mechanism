#include "actors.h"
#include "animalSpecies.h"
#include "config.h"
#include "types.h"

void Actors::attributes_onUpdateGrowthPercent(ActorIndex index)
{
	updateStrength(index);
	updateAgility(index);
	updateDextarity(index);
	updateIntrinsicMass(index);
	canPickUp_updateUnencomberedCarryMass(index);
	combat_update(index);
	move_updateIndividualSpeed(index);
}
[[nodiscard]] AttributeLevel Actors::getStrength(ActorIndex index) const { return m_strength[index]; }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getStrengthBonusOrPenalty(ActorIndex index) const { return m_strengthBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getStrengthModifier(ActorIndex index) const { return m_strengthModifier[index]; }
void Actors::addStrengthModifier(ActorIndex index, float modifier) 
{ 
	m_strengthModifier[index] += modifier; 
	onStrengthChanged(index);
}
void Actors::addStrengthBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty)
{
	m_strengthBonusOrPenalty[index] += bonusOrPenalty;
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
	m_strength[index] = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] AttributeLevel Actors::getDextarity(ActorIndex index) const { return m_dextarity[index]; }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getDextarityBonusOrPenalty(ActorIndex index) const { return m_dextarityBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getDextarityModifier(ActorIndex index) const { return m_dextarityModifier[index]; }
void Actors::addDextarityModifier(ActorIndex index, float modifier) 
{ 
	m_dextarityModifier[index] += modifier; 
	onDextarityChanged(index);
}
void Actors::addDextarityBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty)
{
	m_dextarityBonusOrPenalty[index] += bonusOrPenalty;
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
	m_dextarity[index] = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] AttributeLevel Actors::getAgility(ActorIndex index) const { return m_agility[index]; }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getAgilityBonusOrPenalty(ActorIndex index) const { return m_agilityBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getAgilityModifier(ActorIndex index) const { return m_agilityModifier[index]; }
void Actors::addAgilityModifier(ActorIndex index, float modifier) 
{ 
	m_agilityModifier[index] += modifier; 
	onAgilityChanged(index);
}
void Actors::addAgilityBonusOrPenalty(ActorIndex index, AttributeLevelBonusOrPenalty bonusOrPenalty)
{
	m_agilityBonusOrPenalty[index] += bonusOrPenalty;
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
	m_agility[index] = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] Mass Actors::getIntrinsicMass(ActorIndex index) const { return m_mass[index]; }
[[nodiscard]] int32_t Actors::getIntrinsicMassBonusOrPenalty(ActorIndex index) const { return m_massBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getIntrinsicMassModifier(ActorIndex index) const { return m_massModifier[index]; }
void Actors::addIntrinsicMassModifier(ActorIndex index, float modifier) 
{ 
	m_massModifier[index] += modifier; 
	onIntrinsicMassChanged(index);
}
void Actors::addIntrinsicMassBonusOrPenalty(ActorIndex index, uint32_t bonusOrPenalty)
{
	m_massBonusOrPenalty[index] += bonusOrPenalty;
	onIntrinsicMassChanged(index);
}
void Actors::updateIntrinsicMass(ActorIndex index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	Mass max = AnimalSpecies::getMass(species)[0];
	Mass min = AnimalSpecies::getMass(species)[1];
	m_mass[index] = Mass::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}
void Actors::onIntrinsicMassChanged(ActorIndex index)
{
	updateIntrinsicMass(index);
	move_updateIndividualSpeed(index);
	//TODO: mass does not currently affect combat?
	//combat_update(index);
}
