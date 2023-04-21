/*
 * A game map.
 */

#pragma once

#include <shared_mutex>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <list>

#include "lib/BS_thread_pool_light.hpp"

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


class baseArea
{
public:
	uint32_t m_sizeX;
	uint32_t m_sizeY;
	uint32_t m_sizeZ;
	std::vector<std::vector<std::vector<BLOCK>>> m_blocks;
	LocationBuckets m_locationBuckets;
	std::list<FluidGroup> m_fluidGroups;
	std::unordered_set<FluidGroup*> m_unstableFluidGroups;
	std::unordered_set<BLOCK*> m_caveInCheck;
	std::vector<RouteRequest> m_routeRequestQueue;
	std::vector<VisionRequest> m_visionRequestQueue;
	std::shared_mutex m_blockMutex;
	uint32_t m_routeCacheVersion;
	Buckets<ACTOR, s_actorDoVisionInterval> m_visionBuckets;
	std::vector<std::tuple<std::vector<BLOCK*>,uint32_t,uint32_t>> m_caveInData;
	std::unordered_set<FluidGroup*> m_setStable;
	std::unordered_set<FluidGroup*> m_toDestroy;
	std::list<VisionCuboid> m_visionCuboids;
	bool m_visionCuboidsActive;
	EventSchedule m_eventSchedule;

	void expireRouteCache();

	// Create blocks and store adjacent
	baseArea(uint32_t x, uint32_t y, uint32_t z);

	void readStep();
	void writeStep();
	
	// Create a scheduled event when the actor should move.
	void scheduleMove(ACTOR& actor);
	
	// Add to moving actors and call scheduleMove.
	void registerActorMoving(ACTOR& actor);
	// Remove from moving actors.
	void unregisterActorMoving(ACTOR& actor);

	// RegisterActor, add to visionBucket.
	void registerActor(ACTOR& actor);
	void unregisterActor(ACTOR& actor);
	
	// Emplace a routeRequest object on the queue.
	void registerRouteRequest(ACTOR& actor, bool detour = false);
	// Emplace a visionRequest object on the queue.
	void registerVisionRequest(ACTOR& actor);

	// Create a fluid group.
	FluidGroup* createFluidGroup(const FluidType* fluidType, std::unordered_set<BLOCK*>& blocks, bool checkMerge = true);

	// Assign all visible blocks to a visionCuboid, set m_visionCubioidsActive to true.
	void visionCuboidsActivate();

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(BLOCK& block);

	void validateAllFluidGroups();
	std::string toS();

	// User provided code, no route.
	void notifyNoRouteFound(ACTOR& actor);
};
