#include "skillType.h"
SkillTypeId SkillType::byName(std::string name)
{
	auto found = data.m_name.find(name);
	assert(found != data.m_name.end());
	return SkillTypeId::create(found - data.m_name.end());
}
void SkillType::create(SkillTypeParamaters& p)
{
	data.m_name.add(p.name);
	data.m_xpPerLevelModifier.add(p.xpPerLevelModifier);
	data.m_level1Xp.add(p.level1Xp);
}
