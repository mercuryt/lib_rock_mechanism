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
	static AttackType data;
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
	[[nodiscard]] static std::string getName(AttackTypeId id) { return data.m_name.at(id); };
	[[nodiscard]] static uint32_t getArea(AttackTypeId id) { return data.m_area.at(id); };
	[[nodiscard]] static Force getBaseForce(AttackTypeId id) { return data.m_baseForce.at(id); };
	[[nodiscard]] static DistanceInBlocksFractional getRange(AttackTypeId id) { return data.m_range.at(id); };
	[[nodiscard]] static CombatScore getCombatScore(AttackTypeId id) { return data.m_combatScore.at(id); };
	[[nodiscard]] static Step getCoolDown(AttackTypeId id) { return data.m_coolDown.at(id); };
	[[nodiscard]] static bool getProjectile(AttackTypeId id) { return data.m_projectile.at(id); };
	[[nodiscard]] static WoundType getWoundType(AttackTypeId id) { return data.m_woundType.at(id); };
	[[nodiscard]] static SkillTypeId getSkillType(AttackTypeId id) { return data.m_skillType.at(id); };
	[[nodiscard]] static ItemTypeId getProjectileItemType(AttackTypeId id) { return data.m_projectileItemType.at(id); };
};

