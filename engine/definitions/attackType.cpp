#include "definitions/attackType.h"

AttackTypeId AttackType::create(const AttackTypeParamaters& p)
{
	g_attackTypeData.m_name.add(p.name);
	g_attackTypeData.m_area.add(p.area);
	g_attackTypeData.m_baseForce.add(p.baseForce);
	g_attackTypeData.m_range.add(p.range);
	g_attackTypeData.m_combatScore.add(p.combatScore);
	g_attackTypeData.m_coolDown.add(p.coolDown);
	g_attackTypeData.m_projectile.add(p.projectile);
	g_attackTypeData.m_woundType.add(p.woundType);
	g_attackTypeData.m_skillType.add(p.skillType);
	g_attackTypeData.m_projectileItemType.add(p.projectileItemType);
	return AttackTypeId::create(g_attackTypeData.m_name.size() - 1);
}
AttackTypeId AttackType::byName(std::string name)
{
	auto found = g_attackTypeData.m_name.find(name);
	assert(found != g_attackTypeData.m_name.end());
	return AttackTypeId::create(found - g_attackTypeData.m_name.begin());
}
std::string AttackType::getName(const AttackTypeId& id) { return g_attackTypeData.m_name[id]; };
int AttackType::getArea(const AttackTypeId& id) { return g_attackTypeData.m_area[id]; };
Force AttackType::getBaseForce(const AttackTypeId& id) { return g_attackTypeData.m_baseForce[id]; };
DistanceFractional AttackType::getRange(const AttackTypeId& id) { return g_attackTypeData.m_range[id]; };
CombatScore AttackType::getCombatScore(const AttackTypeId& id) { return g_attackTypeData.m_combatScore[id]; };
Step AttackType::getCoolDown(const AttackTypeId& id) { return g_attackTypeData.m_coolDown[id]; };
bool AttackType::getProjectile(const AttackTypeId& id) { return g_attackTypeData.m_projectile[id]; };
WoundType AttackType::getWoundType(const AttackTypeId& id) { return g_attackTypeData.m_woundType[id]; };
SkillTypeId AttackType::getSkillType(const AttackTypeId& id) { return g_attackTypeData.m_skillType[id]; };
ItemTypeId AttackType::getProjectileItemType(const AttackTypeId& id) { return g_attackTypeData.m_projectileItemType[id]; };
