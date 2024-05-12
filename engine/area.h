/*
 * A game map.
 */

#pragma once

#include "installItem.h"
#include "types.h"
#include "block.h"
#include "mistDisperseEvent.h"
#include "cuboid.h"
#include "visionCuboid.h"
#include "dig.h"
#include "construct.h"
#include "craft.h"
#include "targetedHaul.h"	
#include "fire.h"
#include "woodcutting.h"
#include "stocks.h"
#include "fluidSource.h"
#include "area/rain.h"
#include "area/hasActors.h"
#include "area/hasFluidGroups.h"
#include "area/hasItems.h"
#include "area/hasSleepingSpots.h"
//#include "medical.h"

#include <vector>
#include <unordered_set>
#include <tuple>
#include <list>
#include <string>

//struct WorldLocation;

class Simulation;
struct DeserializationMemo;

class Area final
{
	std::vector<Block> m_blocks;
public:
	AreaId m_id;
	std::wstring m_name;
	//WorldLocation* m_worldLocation;
	Simulation& m_simulation;
	DistanceInBlocks m_sizeX;
	DistanceInBlocks m_sizeY;
	DistanceInBlocks m_sizeZ;
	//TODO: make into 1d vector.
	std::unordered_set<Block*> m_caveInCheck;
	std::vector<std::tuple<std::vector<Block*>,uint32_t,uint32_t>> m_caveInData;

	AreaHasTemperature m_hasTemperature;
	AreaHasActors m_hasActors;
	HasPlants m_hasPlants;
	AreaHasFires m_fires;
	HasFarmFields m_hasFarmFields;
	HasHaulTools m_hasHaulTools;
	HasDigDesignations m_hasDigDesignations;
	HasConstructionDesignations m_hasConstructionDesignations;
	AreaHasStockPiles m_hasStockPiles;
	AreaHasStocks m_hasStocks;
	HasCraftingLocationsAndJobs m_hasCraftingLocationsAndJobs;
	AreaHasTargetedHauling m_hasTargetedHauling;
	AreaHasSleepingSpots m_hasSleepingSpots;
	HasWoodCuttingDesignations m_hasWoodCuttingDesignations;
	HasInstallItemDesignations m_hasInstallItemDesignations;
	//AreaHasMedicalPatients m_hasMedicalPatients;
	AreaHasItems m_hasItems;
	AreaHasFluidSources m_fluidSources;
	AreaHasFluidGroups m_hasFluidGroups;
	AreaHasRain m_hasRain;
	std::list<VisionCuboid> m_visionCuboids;
	bool m_visionCuboidsActive;

	// Create blocks and store adjacent
	Area(AreaId id, std::wstring n, Simulation& s, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z);
	Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& s);
	Area(const Area& area) = delete;
	Area(const Area&& area) = delete;
	void setup();

	void readStep();
	void writeStep();
	
	[[nodiscard]] Block& getBlock(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z);
	[[nodiscard]] Block& getMiddleAtGroundLevel();
	[[nodiscard]] Block& getGroundLevel(DistanceInBlocks x, DistanceInBlocks y);
	//Block& getBlockForAdjacentLocation(WorldLocation& location);

	// Assign all visible blocks to a visionCuboid, set m_visionCubioidsActive to true.
	void visionCuboidsActivate();

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(Block& block);
	// To be called periodically by Simulation.
	void updateClimate();

	[[nodiscard]] std::string toS() const;

	[[nodiscard]] Cuboid getZLevel(DistanceInBlocks z);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::vector<Block>& getBlocks() { return m_blocks; }
	[[nodiscard]] size_t getBlockIndex(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z) const;
	[[nodiscard]] size_t getBlockIndex(const Block& block) const;
	[[nodiscard]] std::array<DistanceInBlocks, 3> getCoordinatesForIndex(size_t index) const;

	// Clear all destructor callbacks in preperation for quit or hibernate.
	void clearReservations();

	bool operator==(const Area& other) const { return this == &other; }
	// For testing.
	[[maybe_unused]] void logActorsAndItems() const;
	uint32_t getTotalCountOfItemTypeOnSurface(const ItemType& itemType) const;
};
inline void to_json(Json& data, const Area* const& area){ data = area->m_id; }
