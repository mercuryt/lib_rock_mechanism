#pragma once
// To be used by BodyPartType and ItemType.
struct SkillType;
struct WeaponType
{
	std::vector<AttackType> attackTypes;
	const SkillType* combatSkill;
};
