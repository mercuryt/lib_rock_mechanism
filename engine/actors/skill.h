#pragma once
#include "../config/config.h"
#include "../definitions/skillType.h"
#include "../numericTypes/types.h"
#include "../dataStructures/smallMap.h"

class Skill final
{
public:
	SkillExperiencePoints m_xpForNextLevel;
	SkillExperiencePoints m_xp = SkillExperiencePoints::create(0);
	SkillLevel m_level = SkillLevel::create(0);
	void addXp(const SkillTypeId& skillType, const SkillExperiencePoints& xp);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(Skill, m_xpForNextLevel, m_xp, m_level);
};
class SkillSet final
{
	SmallMap<SkillTypeId, Skill> m_skills;
public:
	void addXp(const SkillTypeId& skillType, const SkillExperiencePoints& xp);
	void insert(const SkillTypeId& skillType, const SkillLevel& level, const SkillExperiencePoints& xp = SkillExperiencePoints::create(0));
	[[nodiscard]] SkillLevel get(const SkillTypeId& skillType) const;
	[[nodiscard]] auto& getSkills() { return m_skills; }
	[[nodiscard]] const auto& getSkills() const { return m_skills; }
	[[nodiscard]] bool empty() const { return m_skills.empty(); }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SkillSet, m_skills);
};
