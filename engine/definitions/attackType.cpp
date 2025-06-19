#include "definitions/attackType.h"

AttackTypeId AttackType::create(const AttackTypeParamaters& p)
{
	attackTypeData.m_name.add(p.name);
	attackTypeData.m_area.add(p.area);
	attackTypeData.m_baseForce.add(p.baseForce);
	attackTypeData.m_range.add(p.range);
	attackTypeData.m_combatScore.add(p.combatScore);
	attackTypeData.m_coolDown.add(p.coolDown);
	attackTypeData.m_projectile.add(p.projectile);
	attackTypeData.m_woundType.add(p.woundType);
	attackTypeData.m_skillType.add(p.skillType);
	attackTypeData.m_projectileItemType.add(p.projectileItemType);
	return AttackTypeId::create(attackTypeData.m_name.size() - 1);
}
AttackTypeId AttackType::byName(std::string name)
{
	auto found = attackTypeData.m_name.find(name);
	assert(found != attackTypeData.m_name.end());
	return AttackTypeId::create(found - attackTypeData.m_name.begin());
}
std::string AttackType::getName(const AttackTypeId& id) { return attackTypeData.m_name[id]; };
uint32_t AttackType::getArea(const AttackTypeId& id) { return attackTypeData.m_area[id]; };
Force AttackType::getBaseForce(const AttackTypeId& id) { return attackTypeData.m_baseForce[id]; };
DistanceInBlocksFractional AttackType::getRange(const AttackTypeId& id) { return attackTypeData.m_range[id]; };
CombatScore AttackType::getCombatScore(const AttackTypeId& id) { return attackTypeData.m_combatScore[id]; };
Step AttackType::getCoolDown(const AttackTypeId& id) { return attackTypeData.m_coolDown[id]; };
bool AttackType::getProjectile(const AttackTypeId& id) { return attackTypeData.m_projectile[id]; };
WoundType AttackType::getWoundType(const AttackTypeId& id) { return attackTypeData.m_woundType[id]; };
SkillTypeId AttackType::getSkillType(const AttackTypeId& id) { return attackTypeData.m_skillType[id]; };
ItemTypeId AttackType::getProjectileItemType(const AttackTypeId& id) { return attackTypeData.m_projectileItemType[id]; };
