#pragma once
#include "dataVector.h"
#include "types.h"
#include "json.h"
struct SkillTypeParamaters final
{
	std::wstring name;
	float xpPerLevelModifier;
	SkillExperiencePoints level1Xp;
};
class SkillType final
{
	StrongVector<std::wstring, SkillTypeId> m_name;
	StrongVector<float, SkillTypeId> m_xpPerLevelModifier;
	StrongVector<SkillExperiencePoints, SkillTypeId> m_level1Xp;
public:
	static void create(const SkillTypeParamaters& p);
	static SkillTypeId byName(std::wstring name);
	[[nodiscard]] static std::wstring getName(const SkillTypeId& id);
	[[nodiscard]] static float getXpPerLevelModifier(const SkillTypeId& id);
	[[nodiscard]] static SkillExperiencePoints getLevel1Xp(const SkillTypeId& id);
	[[nodiscard]] static const StrongVector<std::wstring, SkillTypeId>& getNames();
};
inline SkillType skillTypeData;
