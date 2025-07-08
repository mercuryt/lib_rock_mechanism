#include "hasSpaceDesignations.h"
void AreaHasSpaceDesignationsForFaction::set(const Point3D& point, const SpaceDesignation& designation)
{
	assert(point.exists());
	assert(!m_data[(int)designation].query(point));
	m_data[(int)designation].maybeInsert(point);
}
void AreaHasSpaceDesignationsForFaction::unset(const Point3D& point, const SpaceDesignation& designation)
{
	assert(m_data[(int)designation].query(point));
	m_data[(int)designation].maybeRemove(point);
}
void AreaHasSpaceDesignationsForFaction::maybeSet(const Point3D& point, const SpaceDesignation& designation)
{
	m_data[(int)designation].maybeInsert(point);
}
void AreaHasSpaceDesignationsForFaction::maybeUnset(const Point3D& point, const SpaceDesignation& designation)
{
	m_data[(int)designation].maybeRemove(point);
}
bool AreaHasSpaceDesignationsForFaction::check(const Point3D& point, const SpaceDesignation& designation) const
{
	return m_data[(int)designation].query(point);
}
SpaceDesignation AreaHasSpaceDesignationsForFaction::getDisplayDesignation(const Point3D& point) const
{
	for(int i = 0; i < (int)SpaceDesignation::SPACE_DESIGNATION_MAX; ++i)
		if(check(point, (SpaceDesignation)i))
			return (SpaceDesignation)i;
	return SpaceDesignation::SPACE_DESIGNATION_MAX;
}
bool AreaHasSpaceDesignationsForFaction::empty(const Point3D& point) const
{
	for(int i = 0; i < (int)SpaceDesignation::SPACE_DESIGNATION_MAX; ++i)
		if(check(point, (SpaceDesignation)i))
			return true;
	return false;
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