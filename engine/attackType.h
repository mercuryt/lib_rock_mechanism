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
	static AttackTypeId create(AttackTypeParamaters& p);
	static AttackTypeId byName(std::string name);
	[[nodiscard]] static std::string getName(AttackTypeId id);
	[[nodiscard]] static uint32_t getArea(AttackTypeId id);
	[[nodiscard]] static Force getBaseForce(AttackTypeId id);
	[[nodiscard]] static DistanceInBlocksFractional getRange(AttackTypeId id);
	[[nodiscard]] static CombatScore getCombatScore(AttackTypeId id);
	[[nodiscard]] static Step getCoolDown(AttackTypeId id);
	[[nodiscard]] static bool getProjectile(AttackTypeId id);
	[[nodiscard]] static WoundType getWoundType(AttackTypeId id);
	[[nodiscard]] static SkillTypeId getSkillType(AttackTypeId id);
	[[nodiscard]] static ItemTypeId getProjectileItemType(AttackTypeId id);
};
inline AttackType attackTypeData;
