#include "attackType.h"

void AttackType::create(AttackTypeParamaters& p)
{
	data.m_name.add(p.name);
	data.m_area.add(p.area);
	data.m_baseForce.add(p.baseForce);
	data.m_range.add(p.range);
	data.m_combatScore.add(p.combatScore);
	data.m_coolDown.add(p.coolDown);
	data.m_projectile.add(p.projectile);
	data.m_woundType.add(p.woundType);
	data.m_skillType.add(p.skillType);
	data.m_projectileItemType.add(p.projectileItemType);
}
