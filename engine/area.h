/*
 * A game map.
 */

#pragma once

#include "installItem.h"
#include "types.h"
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
#include "octTree.h"
#include "opacityFacade.h"
#include "visionRequests.h"
#include "sleep.h"
#include "drink.h"
#include "eat.h"
#include "area/rain.h"
#include "area/hasFluidGroups.h"
#include "area/hasSleepingSpots.h"
#include "area/hasBlockDesignations.h"
#include "farmFields.h"
#include "stockpile.h"
#include "actors/grow.h"
//#include "medical.h"

#include <vector>
#include <tuple>
#include <list>
#include <string>

//struct WorldLocation;

class Simulation;
struct DeserializationMemo;
class Plants;

class Area final
{
	// Dependency injection.
	// TODO: Remove unique_ptr for release build.
	std::unique_ptr<Blocks> m_blocks;
	std::unique_ptr<Actors> m_actors;
	std::unique_ptr<Plants> m_plants;
	std::unique_ptr<Items> m_items;
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
	OctTreeRoot m_octTree;
	VisionRequests m_visionRequests;
	OpacityFacade m_opacityFacade;
	AreaHasVisionCuboids m_visionCuboids;
	BlockIndices m_caveInCheck;
	// uint32_t is fall energy.
	std::vector<std::tuple<BlockIndices, DistanceInBlocks, uint32_t>> m_caveInData;
	std::wstring m_name;
	Simulation& m_simulation;
	AreaId m_id;
	// Because Actors, Items, and Plants take Area& as a single constructor argument it is possible to implicitly convert Area into a new empty of any of them.
	// To prevent this we include the bool m_loaded and assert that it is false in each constructor.
	#ifndef NDEBUG
		bool m_loaded = false;
	#endif
	//WorldLocation* m_worldLocation;

	// Create blocks and store adjacent
	Area(AreaId id, std::wstring n, Simulation& s, const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z);
	Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& s);
	Area(const Area& area) = delete;
	Area(const Area&& area) = delete;
	~Area();
	void setup();

	void doStep();
	
	//BlockIndex getBlockForAdjacentLocation(WorldLocation& location);

	// Cavein read/write
	void doStepCaveIn();
	void stepCaveInRead();
	void stepCaveInWrite();
	void registerPotentialCaveIn(const BlockIndex& block);
	// To be called periodically by Simulation.
	void updateClimate();

	[[nodiscard]] std::string toS() const;

	[[nodiscard]] Json toJson() const;
	[[nodiscard]] Blocks& getBlocks() { return *m_blocks.get(); }
	[[nodiscard]] Plants& getPlants() { return *m_plants.get(); }
	[[nodiscard]] Actors& getActors() { return *m_actors.get(); }
	[[nodiscard]] Items& getItems() { return *m_items.get(); }
	[[nodiscard]] const Blocks& getBlocks() const { return *m_blocks.get(); }
	[[nodiscard]] const Plants& getPlants() const { return *m_plants.get(); }
	[[nodiscard]] const Actors& getActors() const { return *m_actors.get(); }
	[[nodiscard]] const Items& getItems() const { return *m_items.get(); }
	// Clear all destructor callbacks in preperation for quit or hibernate.
	void clearReservations();

	[[nodiscard]] bool operator==(const Area& other) const { return this == &other; }
	// For testing.
	[[maybe_unused]] void logActorsAndItems() const;
	[[nodiscard]] Quantity getTotalCountOfItemTypeOnSurface(const ItemTypeId& itemType) const;
};
inline void to_json(Json& data, const Area* const& area){ data = area->m_id; }
inline void to_json(Json& data, const Area& area){ data = area.m_id; }
