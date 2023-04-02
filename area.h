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

#include "fluidGroup.h"
#include "visionRequest.h"
#include "routeRequest.h"
#include "buckets.h"
#include "hasScheduledEvents.h"
#include "locationBuckets.h"


class baseArea : public HasScheduledEvents
{
public:
	uint32_t m_sizeX;
	uint32_t m_sizeY;
	uint32_t m_sizeZ;
	std::vector<std::vector<std::vector<Block>>> m_blocks;
	LocationBuckets m_locationBuckets;
	std::list<FluidGroup> m_fluidGroups;
	std::unordered_set<FluidGroup*> m_unstableFluidGroups;
	std::unordered_set<Block*> m_caveInCheck;
	std::vector<RouteRequest> m_routeRequestQueue;
	std::vector<VisionRequest> m_visionRequestQueue;
	std::shared_mutex m_blockMutex;
	uint32_t m_routeCacheVersion;
	Buckets<Actor, s_actorDoVisionFrequency> m_visionBuckets;
	std::vector<std::tuple<std::vector<Block*>,uint32_t,uint32_t>> m_caveInData;
	std::unordered_set<FluidGroup*> m_setStable;
	std::unordered_set<FluidGroup*> m_toDestroy;

	void expireRouteCache();

	// Create blocks and store adjacent
	baseArea(uint32_t x, uint32_t y, uint32_t z);
	~baseArea();

	void readStep();
	void writeStep();
	
	// Create a scheduled event when the actor should move.
	void scheduleMove(Actor* actor);
	
	// Add to moving actors and call scheduleMove.
	void registerActorMoving(Actor* actor);
	// Remove from moving actors.
	void unregisterActorMoving(Actor* actor);

	// RegisterActor, add to visionBucket.
	void registerActor(Actor* actor);
	void unregisterActor(Actor* actor);
	
	// Emplace a routeRequest object on the queue.
	void registerRouteRequest(Actor* actor);
	// Emplace a visionRequest object on the queue.
	void registerVisionRequest(Actor* actor);

	// Creat a fluid group.
	FluidGroup* createFluidGroup(const FluidType* fluidType, std::unordered_set<Block*>& blocks, bool checkMerge = true);

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(Block* block);

	void validateAllFluidGroups();
	std::string toS();

	// User provided code, no route.
	void notifyNoRouteFound(Actor* actor);

};
