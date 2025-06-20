#pragma once
#include "definitions/attackType.h"
#include "numericTypes/types.h"
#include <vector>
// To be used by BodyPartType and ItemType.
struct WeaponType
{
	const SkillTypeId combatSkill;
	uint32_t skillBonus;
	Step cooldown;
	std::vector<AttackType> attackTypes;
};
