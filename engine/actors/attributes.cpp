#include "actors.h"
#include "definitions/animalSpecies.h"
#include "config/config.h"
#include "numericTypes/types.h"

void Actors::attributes_onUpdateGrowthPercent(const ActorIndex& index)
{
	updateStrength(index);
	updateAgility(index);
	updateDextarity(index);
	updateIntrinsicMass(index);
	canPickUp_updateUnencomberedCarryMass(index);
	combat_update(index);
	move_updateIndividualSpeed(index);
}
[[nodiscard]] AttributeLevel Actors::getStrength(const ActorIndex& index) const { return m_strength[index]; }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getStrengthBonusOrPenalty(const ActorIndex& index) const { return m_strengthBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getStrengthModifier(const ActorIndex& index) const { return m_strengthModifier[index]; }
void Actors::addStrengthModifier(const ActorIndex& index, float modifier)
{
	m_strengthModifier[index] += modifier;
	onStrengthChanged(index);
}
void Actors::setStrengthModifier(const ActorIndex& index, float modifier)
{
	m_strengthModifier[index] = modifier;
	onStrengthChanged(index);
}
void Actors::addStrengthBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty)
{
	m_strengthBonusOrPenalty[index] += bonusOrPenalty;
	onStrengthChanged(index);
}
void Actors::setStrengthBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty)
{
	m_strengthBonusOrPenalty[index] = bonusOrPenalty;
	onStrengthChanged(index);
}
void Actors::onStrengthChanged(const ActorIndex& index)
{
	updateStrength(index);
	canPickUp_updateUnencomberedCarryMass(index);
	combat_update(index);
}
void Actors::updateStrength(const ActorIndex& index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	AttributeLevel max = AnimalSpecies::getStrength(species)[0];
	AttributeLevel min = AnimalSpecies::getStrength(species)[1];
	m_strength[index] = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] AttributeLevel Actors::getDextarity(const ActorIndex& index) const { return m_dextarity[index]; }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getDextarityBonusOrPenalty(const ActorIndex& index) const { return m_dextarityBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getDextarityModifier(const ActorIndex& index) const { return m_dextarityModifier[index]; }
void Actors::addDextarityModifier(const ActorIndex& index, float modifier)
{
	m_dextarityModifier[index] += modifier;
	onDextarityChanged(index);
}
void Actors::setDextarityModifier(const ActorIndex& index, float modifier)
{
	m_dextarityModifier[index] = modifier;
	onDextarityChanged(index);
}
void Actors::addDextarityBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty)
{
	m_dextarityBonusOrPenalty[index] += bonusOrPenalty;
	onDextarityChanged(index);
}
void Actors::setDextarityBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty)
{
	m_dextarityBonusOrPenalty[index] = bonusOrPenalty;
	onDextarityChanged(index);
}
void Actors::onDextarityChanged(const ActorIndex& index)
{
	updateDextarity(index);
	combat_update(index);
}
void Actors::updateDextarity(const ActorIndex& index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	AttributeLevel max = AnimalSpecies::getDextarity(species)[0];
	AttributeLevel min = AnimalSpecies::getDextarity(species)[1];
	m_dextarity[index] = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] AttributeLevel Actors::getAgility(const ActorIndex& index) const { return m_agility[index]; }
[[nodiscard]] AttributeLevelBonusOrPenalty Actors::getAgilityBonusOrPenalty(const ActorIndex& index) const { return m_agilityBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getAgilityModifier(const ActorIndex& index) const { return m_agilityModifier[index]; }
void Actors::addAgilityModifier(const ActorIndex& index, float modifier)
{
	m_agilityModifier[index] += modifier;
	onAgilityChanged(index);
}
void Actors::setAgilityModifier(const ActorIndex& index, float modifier)
{
	m_agilityModifier[index] = modifier;
	onAgilityChanged(index);
}
void Actors::addAgilityBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty)
{
	m_agilityBonusOrPenalty[index] += bonusOrPenalty;
	onAgilityChanged(index);
}
void Actors::setAgilityBonusOrPenalty(const ActorIndex& index, const AttributeLevelBonusOrPenalty& bonusOrPenalty)
{
	m_agilityBonusOrPenalty[index] = bonusOrPenalty;
	onAgilityChanged(index);
}
void Actors::onAgilityChanged(const ActorIndex& index)
{
	updateAgility(index);
	move_updateIndividualSpeed(index);
	combat_update(index);
}
void Actors::updateAgility(const ActorIndex& index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	AttributeLevel max = AnimalSpecies::getAgility(species)[0];
	AttributeLevel min = AnimalSpecies::getAgility(species)[1];
	m_agility[index] = AttributeLevel::create(util::scaleByPercentRange(max.get(), min.get(), grown));
}

[[nodiscard]] Mass Actors::getIntrinsicMass(const ActorIndex& index) const { return m_mass[index]; }
[[nodiscard]] int Actors::getIntrinsicMassBonusOrPenalty(const ActorIndex& index) const { return m_massBonusOrPenalty[index]; }
[[nodiscard]] float Actors::getIntrinsicMassModifier(const ActorIndex& index) const { return m_massModifier[index]; }
void Actors::addIntrinsicMassModifier(const ActorIndex& index, float modifier)
{
	m_massModifier[index] += modifier;
	onIntrinsicMassChanged(index);
}
void Actors::setIntrinsicMassModifier(const ActorIndex& index, float modifier)
{
	m_massModifier[index] = modifier;
	onIntrinsicMassChanged(index);
}
void Actors::addIntrinsicMassBonusOrPenalty(const ActorIndex& index, int bonusOrPenalty)
{
	m_massBonusOrPenalty[index] += bonusOrPenalty;
	onIntrinsicMassChanged(index);
}
void Actors::setIntrinsicMassBonusOrPenalty(const ActorIndex& index, int bonusOrPenalty)
{
	m_massBonusOrPenalty[index] = bonusOrPenalty;
	onIntrinsicMassChanged(index);
}
void Actors::updateIntrinsicMass(const ActorIndex& index)
{
	AnimalSpeciesId species = getSpecies(index);
	Percent grown = getPercentGrown(index);
	const auto& massData = AnimalSpecies::getMass(species);
	Mass max = massData[1];
	Mass min = massData[0];
	int adultMass = util::scaleByFractionRange(min.get(), max.get(), m_adultHeight[index], AnimalSpecies::getHeight(species)[1]);
	m_mass[index] = Mass::create(util::scaleByPercentRange(massData[2].get(), adultMass, grown));
}
void Actors::onIntrinsicMassChanged(const ActorIndex& index)
{
	updateIntrinsicMass(index);
	move_updateIndividualSpeed(index);
	//TODO: mass does not currently affect combat?
	//combat_update(index);
}
CombatScore Actors::attributes_getCombatScore(const ActorIndex& index) const
{
	return CombatScore::create(
		(m_strength[index].get() * Config::pointsOfCombatScorePerUnitOfStrength) +
		(m_agility[index].get() * Config::pointsOfCombatScorePerUnitOfAgility) +
		(m_dextarity[index].get() * Config::pointsOfCombatScorePerUnitOfDextarity)
	);
}
Speed Actors::attributes_getMoveSpeed(const ActorIndex& index) const
{
	return Speed::create(m_agility[index].get() * Config::unitsOfMoveSpeedPerUnitOfAgility);
}