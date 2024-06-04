/*
 * A game map.
 */

#pragma once

#include "installItem.h"
#include "types.h"
#include "blocks/blocks.h"
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
#include "terrainFacade.h"
#include "area/rain.h"
#include "area/hasActors.h"
#include "area/hasFluidGroups.h"
#include "area/hasItems.h"
#include "area/hasSleepingSpots.h"
#include "area/hasBlockDesignations.h"
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
	Blocks m_blocks;
public:
	AreaHasTemperature m_hasTemperature;
	AreaHasTerrainFacades m_hasTerrainFacades;
	AreaHasActors m_hasActors;
	AreaHasPlants m_hasPlants;
	AreaHasFires m_fires;
	AreaHasFarmFields m_hasFarmFields;
	AreaHasHaulTools m_hasHaulTools;
	AreaHasDigDesignations m_hasDigDesignations;
	AreaHasConstructionDesignations m_hasConstructionDesignations;
	AreaHasStockPiles m_hasStockPiles;
	AreaHasStocks m_hasStocks;
	AreaHasCraftingLocationsAndJobs m_hasCraftingLocationsAndJobs;
	AreaHasTargetedHauling m_hasTargetedHauling;
	AreaHasSleepingSpots m_hasSleepingSpots;
	AreaHasWoodCuttingDesignations m_hasWoodCuttingDesignations;
	AreaHasInstallItemDesignations m_hasInstallItemDesignations;
	//AreaHasMedicalPatients m_hasMedicalPatients;
	AreaHasItems m_hasItems;
	AreaHasFluidSources m_fluidSources;
	AreaHasFluidGroups m_hasFluidGroups;
	AreaHasRain m_hasRain;
	AreaHasBlockDesignations m_blockDesignations;
	//TODO: make into vector?
	std::unordered_set<BlockIndex> m_caveInCheck;
	std::vector<std::tuple<std::vector<BlockIndex>,uint32_t,uint32_t>> m_caveInData;
	std::wstring m_name;
	Simulation& m_simulation;
	AreaId m_id;
	//WorldLocation* m_worldLocation;

	// Create blocks and store adjacent
	Area(AreaId id, std::wstring n, Simulation& s, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z);
	Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& s);
	Area(const Area& area) = delete;
	Area(const Area&& area) = delete;
	void setup();

	void readStep();
	void writeStep();
	
	//BlockIndex getBlockForAdjacentLocation(WorldLocation& location);

	// Cavein read/write
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(BlockIndex block);
	// To be called periodically by Simulation.
	void updateClimate();

	[[nodiscard]] std::string toS() const;

	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Blocks& getBlocks() { return m_blocks; }
	[[nodiscard]] const Blocks& getBlocks() const { return m_blocks; }
	//TODO: Move this somewhere else.
	[[nodiscard]] Facing getFacingForAdjacentOffset(uint8_t adjacentOffset) const;

	// Clear all destructor callbacks in preperation for quit or hibernate.
	void clearReservations();

	[[nodiscard]] bool operator==(const Area& other) const { return this == &other; }
	// For testing.
	[[maybe_unused]] void logActorsAndItems() const;
	[[nodiscard]] Quantity getTotalCountOfItemTypeOnSurface(const ItemType& itemType) const;
};
inline void to_json(Json& data, const Area* const& area){ data = area->m_id; }
