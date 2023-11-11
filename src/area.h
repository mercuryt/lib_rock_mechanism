/*
 * A game map.
 */

#pragma once

#include "block.h"
#include "fluidGroup.h"
#include "mistDisperseEvent.h"
#include "cuboid.h"
#include "visionCuboid.h"
#include "dig.h"
#include "construct.h"
#include "craft.h"
#include "rain.h"
#include "datetime.h"
#include "simulation.h"	
#include "targetedHaul.h"	

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <list>

class Area final
{
public:
	Simulation& m_simulation;
	uint32_t m_sizeX;
	uint32_t m_sizeY;
	uint32_t m_sizeZ;
	std::vector<std::vector<std::vector<Block>>> m_blocks;
	std::unordered_set<Block*> m_caveInCheck;
	std::vector<std::tuple<std::vector<Block*>,uint32_t,uint32_t>> m_caveInData;

	AreaHasTemperature m_areaHasTemperature;
	AreaHasActors m_hasActors;
	HasPlants m_hasPlants;
	HasFires m_fires;
	HasFarmFields m_hasFarmFields;
	HasHaulTools m_hasHaulTools;
	HasDigDesignations m_hasDiggingDesignations;
	HasConstructionDesignations m_hasConstructionDesignations;
	HasStockPiles m_hasStockPiles;
	HasCraftingLocationsAndJobs m_hasCraftingLocationsAndJobs;
	AreaHasTargetedHauling m_targetedHauling;
	HasSleepingSpots m_hasSleepingSpots;
	//TODO: HasItems
	AreaHasItems m_hasItems;
	//TODO: HasFluidGroups.
	std::list<FluidGroup> m_fluidGroups;
	std::unordered_set<FluidGroup*> m_unstableFluidGroups;
	std::unordered_set<FluidGroup*> m_setStable;
	std::unordered_set<FluidGroup*> m_toDestroy;
	AreaHasRain m_hasRain;
	std::list<VisionCuboid> m_visionCuboids;
	bool m_visionCuboidsActive;

	// Create blocks and store adjacent
	Area(Simulation& s, uint32_t x, uint32_t y, uint32_t z);

	void readStep();
	void writeStep();
	
	// Create a fluid group.
	FluidGroup* createFluidGroup(const FluidType& fluidType, std::unordered_set<Block*>& blocks, bool checkMerge = true);

	// Assign all visible blocks to a visionCuboid, set m_visionCubioidsActive to true.
	void visionCuboidsActivate();

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(Block& block);
	// Update time.
	void setDateTime(DateTime now);

	void validateAllFluidGroups();
	std::string toS();

	Cuboid getZLevel(uint32_t z);
	// For testing.
	[[maybe_unused]] void logActorsAndItems() const;
	uint32_t getTotalCountOfItemTypeOnSurface(const ItemType& itemType) const;
};
