#pragma once
#include "../projects/targetedHaul.h"
class Area;
class AreaHasTargetedHauling
{
	Area& m_area;
	std::list<TargetedHaulProject> m_projects;
public:
	AreaHasTargetedHauling(Area& a) : m_area(a) { }
	TargetedHaulProject& begin(const SmallSet<ActorIndex>& actors, const ActorOrItemIndex& target, const Point3D& destination);
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void cancel(TargetedHaulProject& project);
	void complete(TargetedHaulProject& project);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
};