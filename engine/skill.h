#pragma once
#include "config.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <algorithm>
#include <cassert>

struct SkillType final
{
	std::string name;
	float xpPerLevelModifier;
	uint32_t level1Xp;
	// Infastructure.
	bool operator==(const SkillType& skillType) const { return this == &skillType; }
	static const SkillType& byName(const std::string name);
};
inline std::vector<SkillType> skillTypeDataStore;
inline void to_json(Json& data, const SkillType* const& skillType){ data = skillType->name; }
inline void to_json(Json& data, const SkillType& skillType){ data = skillType.name; }
inline void from_json(const Json& data, const SkillType*& skillType){ skillType = &SkillType::byName(data.get<std::string>()); }
class Skill final
{
public:
	const SkillType& m_skillType;
	uint32_t m_level;
	uint32_t m_xp;
	uint32_t m_xpForNextLevel;
	Skill(const SkillType& st, uint32_t l = 0, uint32_t xp = 0) : m_skillType(st), m_level(l), m_xp(xp) { setup(); }
	Skill(const Json& data) : 
		m_skillType(SkillType::byName(data["skillType"].get<std::string>())),
		m_level(data["level"].get<uint32_t>()),
		m_xp(data["xp"].get<uint32_t>()) 
	{ setup();}
	void setup()
	{
		m_xpForNextLevel = m_skillType.level1Xp;
		for(uint32_t i = 0; i < m_level; ++i)
			m_xpForNextLevel *= m_skillType.xpPerLevelModifier;
	}
	Json toJson() const 
	{
		Json data;
		data["skillType"] = m_skillType.name;
		data["level"] = m_level;
		data["xp"] = m_xp;
		return data;
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
class SkillSet final
{
public:
	SkillSet() = default;
	SkillSet(const Json& data)
	{
		for(const Json& skillData : data["skills"])
		{
			const SkillType& skillType = SkillType::byName(skillData["skillType"].get<std::string>());
			auto pair = m_skills.try_emplace(&skillType, skillData);
			assert(pair.second);
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
	std::unordered_map<const SkillType*, Skill> m_skills;
	void addXp(const SkillType& skillType, uint32_t xp)
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
