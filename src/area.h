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

template<class Block, class Actor, class DerivedArea, class FluidType>
class BaseArea
{
public:
	uint32_t m_sizeX;
	uint32_t m_sizeY;
	uint32_t m_sizeZ;
	std::vector<std::vector<std::vector<Block>>> m_blocks;
	LocationBuckets<Block, Actor, DerivedArea> m_locationBuckets;
	std::list<FluidGroup<Block, DerivedArea, FluidType>> m_fluidGroups;
	std::unordered_set<FluidGroup<Block, DerivedArea, FluidType>*> m_unstableFluidGroups;
	std::unordered_set<FluidGroup<Block, DerivedArea, FluidType>*> m_setStable;
	std::unordered_set<FluidGroup<Block, DerivedArea, FluidType>*> m_toDestroy;
	std::unordered_set<Block*> m_caveInCheck;
	std::unordered_map<Block*, int32_t> m_blocksWithChangedTemperature;
	std::vector<std::tuple<std::vector<Block*>,uint32_t,uint32_t>> m_caveInData;
	std::vector<RouteRequest<Block, Actor, DerivedArea>> m_routeRequestQueue;
	uint32_t m_routeCacheVersion;
	std::vector<VisionRequest<Block, Actor, DerivedArea>> m_visionRequestQueue;
	Buckets<Actor, Config::actorDoVisionInterval> m_visionBuckets;
	std::list<VisionCuboid<Block, Actor, DerivedArea>> m_visionCuboids;
	bool m_visionCuboidsActive;
	EventSchedule m_eventSchedule;
	bool m_destroy;

	void expireRouteCache();

	// Create blocks and store adjacent
	BaseArea(uint32_t x, uint32_t y, uint32_t z);
	// Set m_destroy = true to simplify cleanup.
	~BaseArea();

	void readStep();
	void writeStep();
	
	// Create a scheduled event when the actor should move.
	void scheduleMove(Actor& actor);
	
	// Add to moving actors and call scheduleMove.
	void registerActorMoving(Actor& actor);
	// Remove from moving actors.
	void unregisterActorMoving(Actor& actor);

	// RegisterActor, add to visionBucket.
	void registerActor(Actor& actor);
	void unregisterActor(Actor& actor);
	
	// Emplace a routeRequest object on the queue.
	void registerRouteRequest(Actor& actor, bool detour = false);
	// Emplace a visionRequest object on the queue.
	void registerVisionRequest(Actor& actor);

	// Create a fluid group.
	FluidGroup<Block, DerivedArea, FluidType>* createFluidGroup(const FluidType* fluidType, std::unordered_set<Block*>& blocks, bool checkMerge = true);

	// Assign all visible blocks to a visionCuboid, set m_visionCubioidsActive to true.
	void visionCuboidsActivate();

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(Block& block);

	void validateAllFluidGroups();
	std::string toS();

	// Get a z-level for rendering.
	BaseCuboid<Block, Actor, DerivedArea> getZLevel(uint32_t z);
	// User provided code, no route.
	void notifyNoRouteFound(Actor& actor);

};
