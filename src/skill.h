#pragma once
#include <unordered_map>

struct SkillType
{
	std::string name;
	float xpPerLevelModifier;
	uint32_t level1Xp;
};
class Skill
{
public:
	const SkillType& m_skillType;
	uint32_t m_level;
	uint32_t m_xp;
	uint32_t m_xpForNextLevel;
	Skill(const SkillType& st, uint32_t l = 0, uint32_t xp = 0) : m_skillType(st), m_level(l), m_xp(xp)
	{
		m_xpForNextLevel = m_skillType.level1Xp;
		for(uint32_t i = 0; i < m_level; ++i)
		{
			m_xpForNextLevel *= m_skillType.xpPerLevelModifier;
		}
	}
	void addXp(uint32_t xp)
	{
		uint32_t requiredXpForLevelUp = m_xpForNextLevel - m_xp;
		if(requiredXpForLevelUp <= xp)
		{
			xp -= requiredXpForLevelUp;
			m_xp = xp;
			m_xpForNextLevel *= m_skillType.xpPerLevelModifier;
			++m_level;
		}
		else
			m_xp += xp;
	}
};
class SkillSet
{
public:
	std::unordered_map<const SkillType*, Skill> m_skills;
	void addXp(const SkillType& skillType, uint32_t& xp)
	{
		const auto& found = m_skills.find(&skillType);
		if(found == m_skills.end())
			m_skills.emplace(&skillType, skillType);
		else
			found->second.addXp(xp);
	}
	uint32_t get(const SkillType& skillType) const
	{
		const auto& found = m_skills.find(&skillType);
		if(found == m_skills.end())
			return 0;
		else
			return found->second.m_level;
	}
};
