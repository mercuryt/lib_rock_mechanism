#pragma once
#include "attackType.h"
#include <vector>
// To be used by BodyPartType and ItemType.
struct SkillType;
struct WeaponType
{
	const SkillType* combatSkill;
	uint32_t skillBonus;
	Step cooldown;
	std::vector<AttackType> attackTypes;
};
