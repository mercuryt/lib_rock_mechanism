/*
 * A game map.
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <list>

#include "buckets.h"
#include "locationBuckets.h"
#include "shape.h"
#include "visionRequest.h"
#include "routeRequest.h"
#include "fluidGroup.h"
#include "eventSchedule.h"
#include "moveEvent.h"
#include "mistDisperseEvent.h"
#include "cuboid.h"
#include "visionCuboid.h"


class BaseArea
{
public:
	uint32_t m_sizeX;
	uint32_t m_sizeY;
	uint32_t m_sizeZ;
	std::vector<std::vector<std::vector<DerivedBlock>>> m_blocks;
	LocationBuckets m_locationBuckets;
	std::list<FluidGroup> m_fluidGroups;
	std::unordered_set<FluidGroup*> m_unstableFluidGroups;
	std::unordered_set<FluidGroup*> m_setStable;
	std::unordered_set<FluidGroup*> m_toDestroy;
	std::unordered_set<DerivedBlock*> m_caveInCheck;
	std::vector<std::tuple<std::vector<DerivedBlock*>,uint32_t,uint32_t>> m_caveInData;
	std::vector<RouteRequest> m_routeRequestQueue;
	uint32_t m_routeCacheVersion;
	std::vector<VisionRequest> m_visionRequestQueue;
	Buckets<DerivedActor, s_actorDoVisionInterval> m_visionBuckets;
	std::list<VisionCuboid> m_visionCuboids;
	bool m_visionCuboidsActive;
	EventSchedule m_eventSchedule;

	void expireRouteCache();

	// Create blocks and store adjacent
	BaseArea(uint32_t x, uint32_t y, uint32_t z);

	void readStep();
	void writeStep();
	
	// Create a scheduled event when the actor should move.
	void scheduleMove(DerivedActor& actor);
	
	// Add to moving actors and call scheduleMove.
	void registerActorMoving(DerivedActor& actor);
	// Remove from moving actors.
	void unregisterActorMoving(DerivedActor& actor);

	// RegisterActor, add to visionBucket.
	void registerActor(DerivedActor& actor);
	void unregisterActor(DerivedActor& actor);
	
	// Emplace a routeRequest object on the queue.
	void registerRouteRequest(DerivedActor& actor, bool detour = false);
	// Emplace a visionRequest object on the queue.
	void registerVisionRequest(DerivedActor& actor);

	// Create a fluid group.
	FluidGroup* createFluidGroup(const FluidType* fluidType, std::unordered_set<DerivedBlock*>& blocks, bool checkMerge = true);

	// Assign all visible blocks to a visionCuboid, set m_visionCubioidsActive to true.
	void visionCuboidsActivate();

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(DerivedBlock& block);

	void validateAllFluidGroups();
	std::string toS();

	// Get a z-level for rendering.
	Cuboid getZLevel(uint32_t z);
	// User provided code, no route.
	void notifyNoRouteFound(DerivedActor& actor);

};
