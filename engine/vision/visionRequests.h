#pragma once
#include "config.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"
#include "dataStructures/smallSet.h"
#include "reference.h"
#include "geometry/point3D.h"
#include <cassert>
#include <cstdint>
#include <vector>
class Area;
struct VisionRequest
{
	OccupiedBlocksForHasShape occupied;
	SmallSet<ActorReference> canSee = {};
	SmallSet<ActorReference> canBeSeenBy = {};
	Point3D coordinates;
	BlockIndex location;
	VisionCuboidId cuboid;
	ActorReference actor;
	DistanceInBlocks range;
	Facing4 facing;
	VisionRequest(const Point3D& _coordinates, const BlockIndex& _location, const VisionCuboidId& _cuboid, const ActorReference& _actor, const DistanceInBlocks _range, const Facing4& _facing, const OccupiedBlocksForHasShape& _occupied) :
		occupied(_occupied), coordinates(_coordinates), location(_location), cuboid(_cuboid), actor(_actor), range(_range), facing(_facing) { }
	struct hash { [[nodiscard]] static bool operator()(const VisionRequest& request) { return request.actor.getReferenceIndex().get(); }};
	[[nodiscard]] bool operator==(const VisionRequest& visionRequest) const { return visionRequest.actor == actor; }
	[[nodiscard]] bool operator!=(const VisionRequest& visionRequest) const { return visionRequest.actor != actor; }
};
class VisionRequests final
{
	SmallSet<VisionRequest> m_data;
	Area& m_area;
	DistanceInBlocks m_largestRange = DistanceInBlocks::create(0);
public:
	VisionRequests(Area& area);
	void create(const ActorReference& actor);
	void maybeCreate(const ActorReference& actor);
	void cancelIfExists(const ActorReference& actor);
	void readStepSegment(const uint& begin, const uint& end);
	void doStep();
	void readStep();
	void writeStep();
	void clear();
	void maybeGenerateRequestsForAllWithLineOfSightTo(const BlockIndex& block);
	void maybeGenerateRequestsForAllWithLineOfSightToAny(const std::vector<BlockIndex>& blocks);
	void maybeUpdateCuboid(const ActorReference& actor, const VisionCuboidId& newCuboid);
	[[nodiscard]] bool maybeUpdateRange(const ActorReference& actor, const DistanceInBlocks& range);
	bool maybeUpdateLocation(const ActorReference& actor, const BlockIndex& location);
	// These accessors get data from the internal DataVectors and assert they are still matching the cannonical data in Actors and AreaHasVisionCuboids.
	[[nodiscard]] size_t size() const { return m_data.size(); }
};