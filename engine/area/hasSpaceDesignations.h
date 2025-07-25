#pragma once

#include "../designations.h"
#include "../numericTypes/types.h"
#include "../numericTypes/idTypes.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/rtreeBoolean.h"

class Area;
struct DeserializationMemo;
class AreaHasSpaceDesignationsForFaction final
{
	std::array<RTreeBoolean, (int)SpaceDesignation::SPACE_DESIGNATION_MAX> m_data;
public:
	void set(const Point3D& point, const SpaceDesignation& designation);
	void unset(const Point3D& point, const SpaceDesignation& designation);
	void maybeUnset(const Point3D& point, const SpaceDesignation& designation);
	void maybeSet(const Point3D& point, const SpaceDesignation& designation);
	[[nodiscard]] bool check(const Point3D& point, const SpaceDesignation& designation) const;
	[[nodiscard]] SpaceDesignation getDisplayDesignation(const Point3D& point) const;
	[[nodiscard]] bool empty(const Point3D& point) const;
	[[nodiscard]] std::vector<SpaceDesignation> getForPoint(const Point3D& point) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasSpaceDesignationsForFaction, m_data);
};
class AreaHasSpaceDesignations final
{
	SmallMap<FactionId, AreaHasSpaceDesignationsForFaction> m_data;
public:
	void registerFaction(const FactionId& faction) { m_data.emplace(faction); }
	void maybeRegisterFaction(const FactionId& faction) { if(!contains(faction)) m_data.emplace(faction); }
	void unregisterFaction(const FactionId& faction) { m_data.erase(faction); }
	[[nodiscard]] bool contains(const FactionId& faction) const { return m_data.contains(faction); }
	[[nodiscard]] AreaHasSpaceDesignationsForFaction& getForFaction(const FactionId& faction) { return m_data[faction]; }
	[[nodiscard]] const AreaHasSpaceDesignationsForFaction& getForFaction(const FactionId& faction) const { return m_data[faction]; }
	[[nodiscard]] AreaHasSpaceDesignationsForFaction& maybeRegisterAndGetForFaction(const FactionId& faction);
	[[nodiscard]] const AreaHasSpaceDesignationsForFaction& maybeRegisterAndGetForFaction(const FactionId& faction) const;
	[[nodiscard]] std::vector<std::pair<FactionId, SpaceDesignation>> getForPoint(const Point3D& point) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasSpaceDesignations, m_data);
};