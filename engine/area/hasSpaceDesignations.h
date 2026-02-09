#pragma once

#include "../designations.h"
#include "../numericTypes/types.h"
#include "../numericTypes/idTypes.h"
#include "../dataStructures/smallMap.h"
#include "../dataStructures/rtreeBoolean.hpp"

class Area;
struct DeserializationMemo;
class AreaHasSpaceDesignationsForFaction final
{
	std::array<RTreeBoolean, (int)SpaceDesignation::SPACE_DESIGNATION_MAX> m_data;
public:
	void set(const auto& shape, const SpaceDesignation& designation) { assert(shape.exists()); assert(!m_data[(int)designation].query(shape)); maybeSet(shape, designation); }
	void unset(const auto& shape, const SpaceDesignation& designation) { assert(m_data[(int)designation].query(shape)); maybeUnset(shape, designation); }
	void maybeUnset(const auto& shape, const SpaceDesignation& designation) { m_data[(int)designation].maybeRemove(shape); }
	void maybeSet(const auto& shape, const SpaceDesignation& designation) { m_data[(int)designation].maybeInsert(shape); }
	void prepare() { for(auto& rtree : m_data) rtree.prepare(); }
	void queryForEach(auto& shape, auto&& action) const
	{
		for(int designation = 0; designation != (int)SpaceDesignation::SPACE_DESIGNATION_MAX; ++designation)
			m_data[designation].queryForEach(shape, action);
	}
	[[nodiscard]] bool any(const SpaceDesignation& designation) const;
	[[nodiscard]] bool canPrepare() const;
	[[nodiscard]] bool check(const auto& shape, const SpaceDesignation& designation) const { return m_data[(int)designation].query(shape); }
	[[nodiscard]] Point3D queryPoint(const auto& shape, const SpaceDesignation& designation) const { return m_data[(int)designation].queryGetPoint(shape); }
	[[nodiscard]] Point3D queryPointWithCondition(const auto& shape, const SpaceDesignation& designation, auto&& condition) const { return m_data[(int)designation].queryGetPointWithCondition(shape, condition); }
	[[nodiscard]] std::vector<SpaceDesignation> getForPoint(const Point3D& point) const;
	[[nodiscard]] SpaceDesignation getDisplayDesignation(const Point3D& point) const;
	[[nodiscard]] const RTreeBoolean& getForDesignation(const SpaceDesignation& designation) const;
	[[nodiscard]] Cuboid getCuboidWithDesignationAndCondition(const SpaceDesignation& designation, const auto& shape, auto&& condition)
	{
		return m_data[(int)designation].queryGetLeafWithCondition(shape, condition);
	};
	[[nodiscard]] Cuboid getCuboidWithDesignation(const SpaceDesignation& designation, const auto& shape) const { return m_data[(int)designation].queryGetLeaf(shape); }
	[[nodiscard]] bool empty(const auto& shape) const
	{
		for(int i = 0; i < (int)SpaceDesignation::SPACE_DESIGNATION_MAX; ++i)
			if(check(shape, (SpaceDesignation)i))
				return true;
		return false;
	}
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasSpaceDesignationsForFaction, m_data);
};
class AreaHasSpaceDesignations final
{
	SmallMap<FactionId, AreaHasSpaceDesignationsForFaction> m_data;
public:
	void registerFaction(const FactionId& faction) { m_data.emplace(faction); }
	void maybeRegisterFaction(const FactionId& faction) { if(!contains(faction)) m_data.emplace(faction); }
	void unregisterFaction(const FactionId& faction) { m_data.erase(faction); }
	void prepare() { for(auto& pair : m_data) pair.second.prepare(); }
	void queryForEachForFactionIfExists(const FactionId& faction, const auto& shape, auto&& action)
	{
		auto found = m_data.find(faction);
		if(found == m_data.end())
			return;
		found->second.queryForEach(shape, action);
	}
	[[nodiscard]] bool contains(const FactionId& faction) const { return m_data.contains(faction); }
	[[nodiscard]] bool canPrepare() const;
	[[nodiscard]] AreaHasSpaceDesignationsForFaction& getForFaction(const FactionId& faction) { return m_data.getOrCreate(faction); }
	[[nodiscard]] const AreaHasSpaceDesignationsForFaction& getForFaction(const FactionId& faction) const { return m_data[faction]; }
	[[nodiscard]] AreaHasSpaceDesignationsForFaction& maybeRegisterAndGetForFaction(const FactionId& faction);
	[[nodiscard]] const AreaHasSpaceDesignationsForFaction& maybeRegisterAndGetForFaction(const FactionId& faction) const;
	[[nodiscard]] std::vector<std::pair<FactionId, SpaceDesignation>> getForPoint(const Point3D& point) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(AreaHasSpaceDesignations, m_data);
};