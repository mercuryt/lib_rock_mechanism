#include "skill.h"
const SkillType& SkillType::byName(const std::string name)
{
	auto found = std::ranges::find(skillTypeDataStore, name, &SkillType::name);
	assert(found != skillTypeDataStore.end());
	return *found;
}
