#include "hasSpaceDesignations.h"
bool AreaHasSpaceDesignationsForFaction::any(const SpaceDesignation& designation) const
{
	return !m_data[(uint8_t)designation].empty();
}
std::vector<SpaceDesignation> AreaHasSpaceDesignationsForFaction::getForPoint(const Point3D& point) const
{
	std::vector<SpaceDesignation> output;
	output.reserve(uint(SpaceDesignation::SPACE_DESIGNATION_MAX) / 2);
	for(auto designation = SpaceDesignation(0); designation != SpaceDesignation::SPACE_DESIGNATION_MAX; designation = SpaceDesignation((uint)designation + 1))
		if(check(point, designation))
			output.push_back(designation);
	return output;
}
const RTreeBoolean& AreaHasSpaceDesignationsForFaction::getForDesignation(const SpaceDesignation& designation) const { return m_data[(uint8_t)designation]; }
AreaHasSpaceDesignationsForFaction& AreaHasSpaceDesignations::maybeRegisterAndGetForFaction(const FactionId& faction)
{
	auto found = m_data.find(faction);
	if(found != m_data.end())
		return found.second();
	registerFaction(faction);
	return m_data[faction];
}
const AreaHasSpaceDesignationsForFaction& AreaHasSpaceDesignations::maybeRegisterAndGetForFaction(const FactionId& faction) const
{
	return const_cast<AreaHasSpaceDesignations&>(*this).maybeRegisterAndGetForFaction(faction);
}
std::vector<std::pair<FactionId, SpaceDesignation>> AreaHasSpaceDesignations::getForPoint(const Point3D& point) const
{
	std::vector<std::pair<FactionId, SpaceDesignation>> output;
	for(const auto& [faction, forFaction] : m_data)
		for(const auto& designation : forFaction.getForPoint(point))
			output.emplace_back(faction, designation);
	return output;
}