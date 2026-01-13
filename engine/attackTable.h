#pragma once
#include <vector>
#include <utility>
#include "numericTypes/types.h"
#include "numericTypes/index.h"

struct Attack final
{
	AttackTypeId attackType;
	MaterialTypeId materialType;
	ItemIndex item; // Can be null for natural weapons.
	[[nodiscard]] bool isNonLethalAgainst(const AnimalSpeciesId& species);
};

class AttackTable
{
	std::vector<std::pair<CombatScore, Attack>> m_data;
public:
	void add(const CombatScore& score, const Attack& attack);
	void clear();
	[[nodiscard]] bool empty() const;
	[[nodiscard]] size_t size() const;
	// Return total score and max distance.
	[[nodiscard]] std::pair<CombatScore, DistanceFractional> build();
	[[nodiscard]] const Attack& getForCombatScoreDifference(const CombatScore& difference) const;
};