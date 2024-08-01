#pragma once
#include "types.h"
#include "json.h"
struct SkillType final
{
	std::string name;
	float xpPerLevelModifier;
	SkillExperiencePoints level1Xp;
	// Infastructure.
	bool operator==(const SkillType& skillType) const { return this == &skillType; }
	static const SkillType& byName(const std::string name);
};
inline std::vector<SkillType> skillTypeDataStore;
inline void to_json(Json& data, const SkillType* const& skillType){ data = skillType->name; }
inline void to_json(Json& data, const SkillType& skillType){ data = skillType.name; }
inline void from_json(const Json& data, const SkillType*& skillType){ skillType = &SkillType::byName(data.get<std::string>()); }
