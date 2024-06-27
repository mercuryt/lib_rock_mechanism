/*
 * A game map.
 */

#pragma once

#include "installItem.h"
#include "plants.h"
#include "items/items.h"
#include "actors/actors.h"
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
#include "locationBuckets.h"
#include "opacityFacade.h"
#include "visionFacade.h"
#include "sleep.h"
#include "drink.h"
#include "eat.h"
#include "area/rain.h"
#include "area/hasFluidGroups.h"
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
	Actors m_actors;
	Plants m_plants;
	Items m_items;
public:
	EventSchedule m_eventSchedule;
	ThreadedTaskEngine m_threadedTaskEngine;
	AreaHasTemperature m_hasTemperature;
	AreaHasTerrainFacades m_hasTerrainFacades;
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
	AreaHasFluidSources m_fluidSources;
	AreaHasFluidGroups m_hasFluidGroups;
	AreaHasRain m_hasRain;
	AreaHasBlockDesignations m_blockDesignations;
	LocationBuckets m_locationBuckets;
	VisionFacadeBuckets m_visionFacadeBuckets;
	OpacityFacade m_opacityFacade;
	AreaHasVisionCuboids m_visionCuboids;
	VisionFacade m_visionFacade;
	//TODO: make into vector?
	std::unordered_set<BlockIndex> m_caveInCheck;
	// uint32_t is fall energy.
	std::vector<std::tuple<std::vector<BlockIndex>, DistanceInBlocks, uint32_t>> m_caveInData;
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

	void doStep();
	
	//BlockIndex getBlockForAdjacentLocation(WorldLocation& location);

	// Cavein read/write
	void doStepCaveIn();
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(BlockIndex block);
	// To be called periodically by Simulation.
	void updateClimate();

	[[nodiscard]] std::string toS() const;

	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Blocks& getBlocks() { return m_blocks; }
	[[nodiscard]] Plants& getPlants() { return m_plants; }
	[[nodiscard]] Actors& getActors() { return m_actors; }
	[[nodiscard]] Items& getItems() { return m_items; }
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
inline void to_json(Json& data, const Area& area){ data = area.m_id; }
