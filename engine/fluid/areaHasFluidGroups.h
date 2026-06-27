#pragma once
#include "fluidGroup.h"
#include "../geometry/cuboidSet.h"
#include "../numericTypes/idTypes.h"
class Area;
struct AreaHasFluidGroups
{
	std::vector<FluidGroup> m_groups;
	// A reference to Area is stored here inorder to deserialize fluid data.
	Area& m_area;
	FluidGroupId m_nextId{0};
	AreaHasFluidGroups(Area& area);
	void doStep();
	void createGroup(const CuboidSet& occupied, int64_t volume, FluidTypeId type);
	void destroyGroup(FluidGroupId id);
	void clearMerged();
	[[nodiscard]] FluidGroup& byId(FluidGroupId id);
	[[nodiscard]] bool hasUnstable() const;
};
void to_json(Json& data, const AreaHasFluidGroups& group);
void from_json(const Json& data, AreaHasFluidGroups& group);