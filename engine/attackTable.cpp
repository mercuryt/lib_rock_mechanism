#include "attackTable.h"
#include "definitions/attackType.h"
#include "definitions/materialType.h"
#include "definitions/animalSpecies.h"
bool Attack::isNonLethalAgainst(const AnimalSpeciesId& species)
{
	const WoundType& woundType = AttackType::getWoundType(attackType);
	if(woundType != WoundType::Bludgeon)
		return false;
	static auto hardnessOfPineWood = MaterialType::getHardness(MaterialType::byName("pine wood"));
	if(MaterialType::getHardness(materialType) > hardnessOfPineWood)
		return false;
	if(AttackType::getBaseForce(attackType).get() > AnimalSpecies::getMass(species)[0].get() * Config::maximumForceRatioToMassForNonLethalAttack)
		return false;
	return true;
}
void AttackTable::add(const CombatScore& score, const Attack& attack)
{
	m_data.emplace_back(score, attack);
}
void AttackTable::clear()
{
	m_data.clear();
}
bool AttackTable::empty() const { return m_data.empty(); }
size_t AttackTable::size() const { return m_data.size(); }
std::pair<CombatScore, DistanceFractional> AttackTable::build()
{
	std::pair<CombatScore, DistanceFractional> output = {CombatScore::create(0), DistanceFractional::create(0)};
	const auto copy = m_data;
	m_data.clear();
	for(const auto& [score, attack] : copy)
		m_data.emplace_back(score, attack);
	// Sort by combat score, low to high.
	std::sort(m_data.begin(), m_data.end(), [](const auto& a, const auto& b){ return a.first < b.first; });
	// Iterate attacks low to high, add running total to each score.
	// Also find max melee attack range.
	for(auto& pair : m_data)
	{
		pair.first += output.first;
		output.first = pair.first;
		DistanceFractional range = AttackType::getRange(pair.second.attackType);
		if(range > output.second)
			output.second = range;
	}
	return output;
}
const Attack& AttackTable::getForCombatScoreDifference(const CombatScore& difference) const
{
	for(auto& pair : m_data)
		if(pair.first > difference)
			return pair.second;
	return m_data.back().second;
}