#pragma once
#include "../config/config.h"
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "../dataStructures/smallSet.h"
#include "../reference.h"
#include "../geometry/point3D.h"
#include <cassert>
#include <cstdint>
#include <vector>
class Area;
struct VisionRequest
{
	CuboidSet occupied;
	SmallSet<ActorReference> canSee = {};
	SmallSet<ActorReference> canBeSeenBy = {};
	Point3D location;
	ActorReference actor;
	Distance range;
	Facing4 facing;
	VisionRequest(const Point3D& _location, const ActorReference& _actor, const Distance _range, const Facing4& _facing, const CuboidSet& _occupied) :
		occupied(_occupied), location(_location), actor(_actor), range(_range), facing(_facing) { }
	struct hash { [[nodiscard]] static bool operator()(const VisionRequest& request) { return request.actor.getReferenceIndex().get(); }};
	[[nodiscard]] bool operator==(const VisionRequest& visionRequest) const { return visionRequest.actor == actor; }
	[[nodiscard]] bool operator!=(const VisionRequest& visionRequest) const { return visionRequest.actor != actor; }
};
class VisionRequests final
{
	SmallSet<VisionRequest> m_data;
	Area& m_area;
	Distance m_largestRange = Distance::create(0);
public:
	VisionRequests(Area& area);
	void create(const ActorReference& actor);
	void maybeCreate(const ActorReference& actor);
	void cancelIfExists(const ActorReference& actor);
	void readStepSegment(const int& begin, const int& end);
	void doStep();
	void readStep();
	void writeStep();
	void clear();
	void maybeGenerateRequestsForAllWithLineOfSightTo(const Point3D& point);
	void maybeGenerateRequestsForAllWithLineOfSightToAny(const std::vector<Point3D>& space);
	[[nodiscard]] bool maybeUpdateRange(const ActorReference& actor, const Distance& range);
	bool maybeUpdateLocation(const ActorReference& actor, const Point3D& location);
	[[nodiscard]] size_t size() const { return m_data.size(); }
};