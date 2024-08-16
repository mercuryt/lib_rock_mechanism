#include "skillType.h"
SkillTypeId SkillType::byName(std::string name)
{
	auto found = skillTypeData.m_name.find(name);
	assert(found != skillTypeData.m_name.end());
	return SkillTypeId::create(found - skillTypeData.m_name.end());
}
void SkillType::create(SkillTypeParamaters& p)
{
	skillTypeData.m_name.add(p.name);
	skillTypeData.m_xpPerLevelModifier.add(p.xpPerLevelModifier);
	skillTypeData.m_level1Xp.add(p.level1Xp);
}
std::string SkillType::getName(SkillTypeId id) { return skillTypeData.m_name[id]; }
float SkillType::getXpPerLevelModifier(SkillTypeId id) { return skillTypeData.m_xpPerLevelModifier[id]; }
SkillExperiencePoints SkillType::getLevel1Xp(SkillTypeId id) { return skillTypeData.m_level1Xp[id]; }
