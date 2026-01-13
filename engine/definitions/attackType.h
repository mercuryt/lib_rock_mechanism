#pragma once

#include "../dataStructures/strongVector.h"
#include "../numericTypes/types.h"
#include "woundType.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
struct AttackTypeParamaters final
{
	std::string name;
	int area;
	Force baseForce;
	DistanceFractional range;
	CombatScore combatScore;
	Step coolDown;
	bool projectile;
	WoundType woundType;
	SkillTypeId skillType;
	ItemTypeId projectileItemType;
};

class AttackType final
{
	StrongVector<std::string, AttackTypeId> m_name;
	StrongVector<int, AttackTypeId> m_area;
	StrongVector<Force, AttackTypeId> m_baseForce;
	StrongVector<DistanceFractional, AttackTypeId> m_range;
	StrongVector<CombatScore, AttackTypeId> m_combatScore;
	StrongVector<Step, AttackTypeId> m_coolDown;
	StrongBitSet<AttackTypeId> m_projectile;
	StrongVector<WoundType, AttackTypeId> m_woundType;
	StrongVector<SkillTypeId, AttackTypeId> m_skillType;
	StrongVector<ItemTypeId, AttackTypeId> m_projectileItemType;
public:
	static AttackTypeId create(const AttackTypeParamaters& p);
	static AttackTypeId byName(std::string name);
	[[nodiscard]] static std::string getName(const AttackTypeId& id);
	[[nodiscard]] static int getArea(const AttackTypeId& id);
	[[nodiscard]] static Force getBaseForce(const AttackTypeId& id);
	[[nodiscard]] static DistanceFractional getRange(const AttackTypeId& id);
	[[nodiscard]] static CombatScore getCombatScore(const AttackTypeId& id);
	[[nodiscard]] static Step getCoolDown(const AttackTypeId& id);
	[[nodiscard]] static bool getProjectile(const AttackTypeId& id);
	[[nodiscard]] static WoundType getWoundType(const AttackTypeId& id);
	[[nodiscard]] static SkillTypeId getSkillType(const AttackTypeId& id);
	[[nodiscard]] static ItemTypeId getProjectileItemType(const AttackTypeId& id);
};
inline AttackType g_attackTypeData;
