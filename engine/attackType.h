#pragma once

#include "dataVector.h"
#include "woundType.h"
#include "types.h"

#include <string>
#include <vector>
#include <algorithm>
#include <cassert>
struct AttackTypeParamaters final
{
	std::string name;
	uint32_t area;
	Force baseForce;
	DistanceInBlocksFractional range;
	CombatScore combatScore;
	Step coolDown;
	bool projectile;
	WoundType woundType;
	SkillTypeId skillType;
	ItemTypeId projectileItemType;
};

class AttackType final
{
	DataVector<std::string, AttackTypeId> m_name;
	DataVector<uint32_t, AttackTypeId> m_area;
	DataVector<Force, AttackTypeId> m_baseForce;
	DataVector<DistanceInBlocksFractional, AttackTypeId> m_range;
	DataVector<CombatScore, AttackTypeId> m_combatScore;
	DataVector<Step, AttackTypeId> m_coolDown;
	DataBitSet<AttackTypeId> m_projectile;
	DataVector<WoundType, AttackTypeId> m_woundType;
	DataVector<SkillTypeId, AttackTypeId> m_skillType;
	DataVector<ItemTypeId, AttackTypeId> m_projectileItemType;
public:
	static AttackTypeId create(const AttackTypeParamaters& p);
	static AttackTypeId byName(std::string name);
	[[nodiscard]] static std::string getName(const AttackTypeId& id);
	[[nodiscard]] static uint32_t getArea(const AttackTypeId& id);
	[[nodiscard]] static Force getBaseForce(const AttackTypeId& id);
	[[nodiscard]] static DistanceInBlocksFractional getRange(const AttackTypeId& id);
	[[nodiscard]] static CombatScore getCombatScore(const AttackTypeId& id);
	[[nodiscard]] static Step getCoolDown(const AttackTypeId& id);
	[[nodiscard]] static bool getProjectile(const AttackTypeId& id);
	[[nodiscard]] static WoundType getWoundType(const AttackTypeId& id);
	[[nodiscard]] static SkillTypeId getSkillType(const AttackTypeId& id);
	[[nodiscard]] static ItemTypeId getProjectileItemType(const AttackTypeId& id);
};
inline AttackType attackTypeData;
