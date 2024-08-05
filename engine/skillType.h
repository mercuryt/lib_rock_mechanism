#pragma once
#include "dataVector.h"
#include "types.h"
#include "json.h"
struct SkillTypeParamaters final
{
	std::string name;
	float xpPerLevelModifier;
	SkillExperiencePoints level1Xp;
};
class SkillType final
{
	static SkillType data;
	DataVector<std::string, SkillTypeId> m_name;
	DataVector<float, SkillTypeId> m_xpPerLevelModifier;
	DataVector<SkillExperiencePoints, SkillTypeId> m_level1Xp;
public:
	static void create(SkillTypeParamaters& p);
	static SkillTypeId byName(std::string name);
	[[nodiscard]] static std::string getname(SkillTypeId id) { return data.m_name.at(id); }
	[[nodiscard]] static float getxpPerLevelModifier(SkillTypeId id) { return data.m_xpPerLevelModifier.at(id); }
	[[nodiscard]] static SkillExperiencePoints getlevel1Xp(SkillTypeId id) { return data.m_level1Xp.at(id); }
};
