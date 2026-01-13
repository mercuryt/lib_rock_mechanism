#include "hasSpaceDesignations.h"
bool AreaHasSpaceDesignationsForFaction::any(const SpaceDesignation& designation) const
{
	return !m_data[(int)designation].empty();
}
bool AreaHasSpaceDesignationsForFaction::canPrepare() const
{
	for(const auto& rtree : m_data)
		if(rtree.canPrepare())
			return true;
	return false;
}
std::vector<SpaceDesignation> AreaHasSpaceDesignationsForFaction::getForPoint(const Point3D& point) const
{
	std::vector<SpaceDesignation> output;
	output.reserve(int(SpaceDesignation::SPACE_DESIGNATION_MAX) / 2);
	for(auto designation = SpaceDesignation(0); designation != SpaceDesignation::SPACE_DESIGNATION_MAX; designation = SpaceDesignation((int)designation + 1))
		if(check(point, designation))
			output.push_back(designation);
	return output;
}
const RTreeBoolean& AreaHasSpaceDesignationsForFaction::getForDesignation(const SpaceDesignation& designation) const { return m_data[(int)designation]; }
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
bool AreaHasSpaceDesignations::canPrepare() const
{
	for(const auto& pair : m_data)
		if(pair.second.canPrepare())
			return true;
	return false;
}