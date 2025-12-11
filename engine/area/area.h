/*
 * A game map.
 */

#pragma once

#include "../numericTypes/types.h"
#include "../mistDisperseEvent.h"
#include "../geometry/cuboid.h"
#include "../vision/visionCuboid.h"
#include "../craft.h"
#include "../fire.h"
#include "../stocks.h"
#include "../fluidSource.h"
#include "../path/terrainFacade.h"
#include "../dataStructures/octTree.h"
#include "../vision/opacityFacade.h"
#include "../vision/visionRequests.h"
#include "../sleep.h"
#include "../drink.h"
#include "../eat.h"
#include "../farmFields.h"
#include "../actors/grow.h"
#include "../deck.h"
#include "installItem.h"
#include "woodcutting.h"
#include "stockpile.h"
#include "rain.h"
#include "hasFluidGroups.h"
#include "hasSleepingSpots.h"
#include "hasSpaceDesignations.h"
#include "evaporation.h"
#include "exteriorPortals.h"
#include "hasTargetedHauling.h"
#include "hasDigDesignations.h"
#include "hasConstructionDesignations.h"
//#include "medical.h"

#include <vector>
#include <tuple>
#include <list>
#include <string>

//struct WorldLocation;

class Simulation;
struct DeserializationMemo;
#ifdef NDEBUG
	#include "space/space.h"
	#include "actors/actors.h"
	#include "items/items.h"
	#include "plants.h"
#else
	class Space;
	class Actors;
	class Items;
	class Plants;
#endif

class Area final
{
	// Dependency injection.
	#ifdef NDEBUG
		Space m_space;
		Actors m_actors;
		Plants m_plants;
		Items m_items;
	#else
		std::unique_ptr<Space> m_space;
		std::unique_ptr<Actors> m_actors;
		std::unique_ptr<Plants> m_plants;
		std::unique_ptr<Items> m_items;
	#endif
public:
	EventSchedule m_eventSchedule;
	ThreadedTaskEngine m_threadedTaskEngine;
	AreaHasTemperature m_hasTemperature;
	AreaHasTerrainFacades m_hasTerrainFacades;
	AreaHasFires m_fires;
	// To be updated when an actor becomes a soldier, stops being a soldier, or moves while being a soldier.
	// Used by summing for all enemy factions for point or path and comparing delta to PsycologyAttribute::Courage.
	// A failed courage check generates a flee objective.
	SmallMap<FactionId, RTreeData<CombatScore>> m_maliceMap;
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
	AreaHasEvaporation m_hasEvaporation;
	// TODO: make to Space.
	AreaHasSpaceDesignations m_spaceDesignations;
	ActorOctTree m_octTree;
	VisionRequests m_visionRequests;
	OpacityFacade m_opacityFacade;
	AreaHasVisionCuboids m_visionCuboids;
	AreaHasExteriorPortals m_exteriorPortals;
	AreaHasDecks m_decks;
	std::string m_name;
	Simulation& m_simulation;
	AreaId m_id;
	bool m_destroy = false;
	//WorldLocation* m_worldLocation;

	// Create space and store adjacent
	Area(AreaId id, std::string n, Simulation& s, const Distance& x, const Distance& y, const Distance& z);
	Area(const Json& data, DeserializationMemo& deserializationMemo, Simulation& s);
	Area(const Area& area) = delete;
	Area(const Area&& area) = delete;
	~Area();
	void setup();

	void doStep();

	// To be called periodically by Simulation.
	void updateClimate();

	[[nodiscard]] std::string toS() const;
	[[nodiscard]] Json toJson() const;
	#ifdef NDEBUG
		[[nodiscard]] Space& getSpace() { return m_space; }
		[[nodiscard]] Plants& getPlants() { return m_plants; }
		[[nodiscard]] Actors& getActors() { return m_actors; }
		[[nodiscard]] Items& getItems() { return m_items; }
		[[nodiscard]] const Space& getSpace() const { return m_space; }
		[[nodiscard]] const Plants& getPlants() const { return m_plants; }
		[[nodiscard]] const Actors& getActors() const { return m_actors; }
		[[nodiscard]] const Items& getItems() const { return m_items; }
	#else
		[[nodiscard]] Space& getSpace() { assert(m_space != nullptr); return *m_space.get(); }
		[[nodiscard]] Plants& getPlants() { assert(m_plants != nullptr); return *m_plants.get(); }
		[[nodiscard]] Actors& getActors() { assert(m_actors != nullptr); return *m_actors.get(); }
		[[nodiscard]] Items& getItems() { assert(m_items != nullptr); return *m_items.get(); }
		[[nodiscard]] const Space& getSpace() const { assert(m_space != nullptr); return *m_space.get(); }
		[[nodiscard]] const Plants& getPlants() const { assert(m_plants != nullptr); return *m_plants.get(); }
		[[nodiscard]] const Actors& getActors() const { assert(m_actors != nullptr); return *m_actors.get(); }
		[[nodiscard]] const Items& getItems() const { assert(m_items != nullptr); return *m_items.get(); }
	#endif
	// Clear all destructor callbacks in preperation for quit or hibernate.
	void clearReservations();

	[[nodiscard]] bool operator==(const Area& other) const { return this == &other; }
	// For testing.
	[[maybe_unused]] void logActorsAndItems() const;
	[[nodiscard]] Quantity getTotalCountOfItemTypeOnSurface(const ItemTypeId& itemType) const;
};
inline void to_json(Json& data, const Area* const& area){ data = area->m_id; }
inline void to_json(Json& data, const Area& area){ data = area.m_id; }
