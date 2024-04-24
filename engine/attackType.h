#pragma once

#include "woundType.h"
#include "types.h"
struct SkillType;
struct ItemType;

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
struct AttackType
{
	std::string name;
	uint32_t area;
	uint32_t baseForce;
	float range;
	uint32_t combatScore;
	Step coolDown;
	bool projectile;
	const WoundType woundType;
	const SkillType& skillType;
	const ItemType* projectileItemType;
	AttackType(std::string n, uint32_t a, uint32_t bf, float r, uint32_t cs, Step cd, bool p, const WoundType wt, const SkillType& st, const ItemType* pit) :
		name(n), area(a), baseForce(bf), range(r), combatScore(cs), coolDown(cd), projectile(p), woundType(wt), skillType(st), projectileItemType(pit) { }
};
