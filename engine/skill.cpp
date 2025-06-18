#include "skill.h"

void Skill::addXp(const SkillTypeId& skillType, const SkillExperiencePoints& xp)
{
	m_xp += xp;
	if(m_xpForNextLevel <= m_xp)
	{
		SkillExperiencePoints remainder = m_xp - m_xpForNextLevel;
		m_xp = remainder;
		m_xpForNextLevel *= SkillType::getXpPerLevelModifier(skillType);
		++m_level;
	}
}
void SkillSet::addXp(const SkillTypeId& skillType, const SkillExperiencePoints& xp)
{
	auto found = m_skills.find(skillType);
	if(found == m_skills.end())
	{
		m_skills.emplace(skillType, SkillType::getLevel1Xp(skillType));
		// Add rather then emplace starting xp because it may be enough to level up.
		m_skills.back().second.addXp(skillType, xp);
	}
	else
		found->second.addXp(skillType, xp);
}
void SkillSet::insert(const SkillTypeId& skillType, const SkillLevel& level, const SkillExperiencePoints& xp)
{
	assert(!m_skills.contains(skillType));
	m_skills.emplace(skillType, SkillType::getLevel1Xp(skillType), xp, level);
}
SkillLevel SkillSet::get(const SkillTypeId& skillType) const
{
	auto found = m_skills.find(skillType);
	if(found == m_skills.end())
		return SkillLevel::create(0);
	else
		return found->second.m_level;
}