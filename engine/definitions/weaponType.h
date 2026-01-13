#pragma once
#include "definitions/attackType.h"
#include "numericTypes/types.h"
#include <vector>
// To be used by BodyPartType and ItemType.
struct WeaponType
{
	const SkillTypeId combatSkill;
	int32_t skillBonus;
	Step cooldown;
	std::vector<AttackType> attackTypes;
};
