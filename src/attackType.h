#pragma once

#include "woundType.h"
#include "skill.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
struct AttackType
{
	std::string name;
	uint32_t area;
	uint32_t baseForce;
	uint32_t range;
	int32_t skillBonus;
	const WoundType woundType;
	AttackType(std::string n, uint32_t a, uint32_t bf, uint32_t r, int32_t sb, const WoundType& wt) :
		name(n), area(a), baseForce(bf), range(r), skillBonus(sb), woundType(wt) { }
};
