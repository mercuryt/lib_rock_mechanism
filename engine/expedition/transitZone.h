#pragma once
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "../numericTypes/idTypes.h"
#include "../geometry/point3D.h"

class Expedition;
class Area;

namespace expeditionTransitZone
{
	[[nodiscard]] CuboidSet makeTransitZone(const Area& area, Facing6 facing);
	[[nodiscard]] Point3D findArrivalPointInZone(const Area& toArea, const Area& fromArea, Facing6 facing, ActorIndex leader, MoveTypeId expeditionMoveType, const CuboidSet& zone, Point3D center);
	// Set location for leader as well as any mount / vehicle / passengers / followers
	[[nodiscard]] std::pair<ActorOrItemIndex, SmallSet<ActorOrItemIndex>> arriveInZone(Area& toArea, Area& fromArea, Point3D location, Facing4 facing, ActorOrItemIndex index);
	ActorOrItemIndex exitArea(Expedition& expedition, Area& fromArea, ActorOrItemIndex index);
}