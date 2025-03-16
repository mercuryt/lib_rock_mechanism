#pragma once
#include "config.h"
#include "skillType.h"
#include "types.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

class Skill final
{
public:
	SkillTypeId m_skillType;
	SkillLevel m_level;
	SkillExperiencePoints m_xp;
	SkillExperiencePoints m_xpForNextLevel;
	Skill(const SkillTypeId& st, SkillLevel l = SkillLevel::create(0), SkillExperiencePoints xp = SkillExperiencePoints::create(0)) :
		m_skillType(st), m_level(l), m_xp(xp) { setup(); }
	Skill(const Json& data) :
		m_skillType(SkillType::byName(data["skillType"].get<std::string>())),
		m_level(data["level"].get<SkillLevel>()),
		m_xp(data["xp"].get<SkillExperiencePoints>())
	{ setup();}
	void setup()
	{
		m_xpForNextLevel = SkillType::getLevel1Xp(m_skillType);
		for(SkillLevel i = SkillLevel::create(0); i < m_level; ++i)
			m_xpForNextLevel *= SkillType::getXpPerLevelModifier(m_skillType);
	}
	Json toJson() const
	{
		Json data;
		data["skillType"] = SkillType::getName(m_skillType);
		data["level"] = m_level;
		data["xp"] = m_xp;
		return data;
	}
	void addXp(SkillExperiencePoints xp)
	{
		SkillExperiencePoints requiredXpForLevelUp = m_xpForNextLevel - m_xp;
		if(requiredXpForLevelUp <= xp)
		{
			xp -= requiredXpForLevelUp;
			m_xp = xp;
			m_xpForNextLevel *= SkillType::getXpPerLevelModifier(m_skillType);
			++m_level;
		}
		else
			m_xp += xp;
	}
};
class SkillSet final
{
	SkillTypeMap<Skill> m_skills;
public:
	void load(const Json& data)
	{
		for(const Json& skillData : data["skills"])
		{
			SkillTypeId skillType = SkillType::byName(skillData["skillType"].get<std::string>());
			m_skills.emplace(skillType, skillData);
		}
	}
	Json toJson() const
	{
		Json data;
		data["skills"] = Json::array();
		for(auto pair : m_skills)
			data["skills"].push_back(pair.second.toJson());
		return data;
	}
	void addXp(const SkillTypeId& skillType, SkillExperiencePoints xp)
	{
		//TODO: We are searching m_skills twice if a skill with the provided type exists.
		if(!m_skills.contains(skillType))
			m_skills.emplace(skillType, skillType, SkillLevel::create(0), xp);
		else
			m_skills[skillType].addXp(xp);
	}
	SkillLevel get(const SkillTypeId& skillType) const
	{
		const auto found = m_skills.find(skillType);
		if(found == m_skills.end())
			return SkillLevel::create(0);
		else
			return found.second().m_level;
	}
	SkillTypeMap<Skill>& getSkills() { return m_skills; }
	const SkillTypeMap<Skill>& getSkills() const { return m_skills; }
};
