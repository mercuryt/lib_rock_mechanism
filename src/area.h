/*
 * A game map.
 */

#pragma once

#include "block.h"
#include "buckets.h"
#include "locationBuckets.h"
#include "visionRequest.h"
#include "routeRequest.h"
#include "fluidGroup.h"
#include "eventSchedule.h"
#include "mistDisperseEvent.h"
#include "cuboid.h"
#include "visionCuboid.h"
#include "dig.h"
#include "construct.h"
#include "craft.h"
#include "../lib/BS_thread_pool_light.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <list>

class Area
{
public:
	uint32_t m_sizeX;
	uint32_t m_sizeY;
	uint32_t m_sizeZ;
	std::vector<std::vector<std::vector<Block>>> m_blocks;
	std::unordered_set<Block*> m_caveInCheck;
	std::vector<std::tuple<std::vector<Block*>,uint32_t,uint32_t>> m_caveInData;

	AreaHasTemperature m_areaHasTemperature;
	HasPlants m_hasPlants;
	HasFires m_fires;
	HasFarmFields m_hasFarmFields;
	HasHaulTools m_hasHaulTools;
	HasDigDesignations m_hasDiggingDesignations;
	HasConstructionDesignations m_hasConstructionDesignations;
	HasStockPiles m_hasStockPilingDesignations;
	HasCraftingLocationsAndJobs m_hasCraftingLocationsAndJobs;
	//TODO: HasItems
	std::list<Item> m_items;
	//TODO: HasActors
	LocationBuckets m_actorLocationBuckets;
	//TODO: HasFluidGroups.
	std::list<FluidGroup> m_fluidGroups;
	std::unordered_set<FluidGroup*> m_unstableFluidGroups;
	std::unordered_set<FluidGroup*> m_setStable;
	std::unordered_set<FluidGroup*> m_toDestroy;
	//TODO: HasVision.
	std::vector<VisionRequest> m_visionRequestQueue;
	Buckets<Actor, Config::actorDoVisionInterval> m_visionBuckets;
	std::list<VisionCuboid> m_visionCuboids;
	bool m_visionCuboidsActive;


	// Create blocks and store adjacent
	Area(uint32_t x, uint32_t y, uint32_t z);

	void readStep(BS::thread_pool_light pool);
	void writeStep();
	
	
	// RegisterActor, add to visionBucket.
	void registerActor(Actor& actor);
	void unregisterActor(Actor& actor);
	
	// Create a fluid group.
	FluidGroup* createFluidGroup(const FluidType& fluidType, std::unordered_set<Block*>& blocks, bool checkMerge = true);

	// Emplace a visionRequest object on the queue.
	void registerVisionRequest(Actor& actor);
	// Assign all visible blocks to a visionCuboid, set m_visionCubioidsActive to true.
	void visionCuboidsActivate();

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(Block& block);

	void validateAllFluidGroups();
	std::string toS();

	Cuboid getZLevel(uint32_t z);
};
