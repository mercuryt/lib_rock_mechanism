#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/item.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/stockpile.h"
#include "../../engine/materialType.h"
#include "../../engine/project.h"
#include "../../engine/config.h"
#include "../../engine/haul.h"
#include "../../engine/goTo.h"
#include <functional>
#include <memory>
TEST_CASE("stockpile")
{
	const MaterialType& wood = MaterialType::byName("poplar wood");
	const MaterialType& marble = MaterialType::byName("marble");
	[[maybe_unused]] const MaterialType& lead = MaterialType::byName("lead");
	const MaterialType& gold = MaterialType::byName("gold");
	[[maybe_unused]] const MaterialType& iron = MaterialType::byName("iron");
	[[maybe_unused]] const MaterialType& peridotite = MaterialType::byName("peridotite");
	const ItemType& chunk = ItemType::byName("chunk");
	const ItemType& cart = ItemType::byName("cart");
	[[maybe_unused]] const ItemType& boulder = ItemType::byName("boulder");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.registerFaction(faction);
	area.m_hasStocks.addFaction(faction);
	Actor& dwarf1 = simulation.createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"), 
		.location=&area.getBlock(1, 1, 1),
		.hasCloths=false,
		.hasSidearm=false
	});
	dwarf1.setFaction(&faction);
	StockPileObjectiveType objectiveType;
	SUBCASE("basic")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		REQUIRE(objectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		REQUIRE(area.m_hasStockPiles.at(faction).getHaulableItemForAt(dwarf1, chunkLocation));
		// Find item to stockpile, make project.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		Project& project = *dwarf1.m_project;
		REQUIRE(project.hasCandidate(dwarf1));
		// Reserve required.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		// Path to chunk.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, chunkLocation);
		REQUIRE(dwarf1.m_canPickup.isCarrying(chunk1));
		std::function<bool()> predicate = [&](){ return chunk1.m_location == &stockpileLocation; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!objectiveType.canBeAssigned(dwarf1));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("one worker hauls two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation1 = area.getBlock(5, 5, 1);
		Block& stockpileLocation2 = area.getBlock(5, 6, 1);
		Block& chunkLocation1 = area.getBlock(1, 8, 1);
		Block& chunkLocation2 = area.getBlock(9, 8, 1);
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		Item& chunk2 = simulation.createItemGeneric(chunk, marble, 1u);
		chunk1.setLocation(chunkLocation1);
		chunk2.setLocation(chunkLocation2);
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk1) == &stockpile);
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk2) == &stockpile);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		area.m_hasStockPiles.at(faction).addItem(chunk2);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinations().size() == 2);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinationsByStockPile().size() == 1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		std::function<bool()> predicate = [&](){ return chunk1.m_location == &stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(objectiveType.canBeAssigned(dwarf1));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "stockpile");
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinations().size() == 1);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinationsByStockPile().size() == 1);
		// Find the second item and stockpile slot, create project and add dwarf1 as candidate.
		simulation.doStep();
		REQUIRE(dwarf1.m_project->getLocation() == stockpileLocation2);
		predicate = [&](){ return chunk2.m_location == &stockpileLocation2 && dwarf1.m_project == nullptr; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(!objectiveType.canBeAssigned(dwarf1));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("Two workers haul two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation1 = area.getBlock(5, 5, 1);
		Block& stockpileLocation2 = area.getBlock(5, 6, 1);
		Block& chunkLocation1 = area.getBlock(1, 8, 1);
		Block& chunkLocation2 = area.getBlock(9, 8, 1);
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.createItemGeneric(chunk, marble, 1u);
		Item& chunk2 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation1);
		chunk2.setLocation(chunkLocation2);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		area.m_hasStockPiles.at(faction).addItem(chunk2);
		Actor& dwarf2 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.getBlock(1, 2, 1));
		dwarf2.setFaction(&faction);
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(dwarf2));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Both dwarves become candidates for chunk1.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		StockPileProject& project = *static_cast<StockPileProject*>(dwarf1.m_project);
		REQUIRE(project.hasCandidate(dwarf1));
		REQUIRE(project.hasCandidate(dwarf2));
		REQUIRE(static_cast<StockPileObjective&>(dwarf1.m_hasObjectives.getCurrent()).m_project != nullptr);
		// Chunk1 project reserves.
		simulation.doStep();
		REQUIRE(project.getWorkers().contains(&dwarf1));
		REQUIRE(project.getWorkers().contains(&dwarf2));
		REQUIRE(project.reservationsComplete());
		// Dwarf1 creates a haulsubproject, dwarf2 is dismissed from the project.
		simulation.doStep();
		REQUIRE(project.getWorkers().contains(&dwarf1));
		REQUIRE(!project.getWorkers().contains(&dwarf2));
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		REQUIRE(dwarf2.m_project == nullptr);
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "stockpile");
		// Dwarf1 paths to chunk1, dwarf2 becomes a candidate for hauling chunk2.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		REQUIRE(chunk1.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
		REQUIRE(dwarf2.m_project != nullptr);
		REQUIRE(dwarf2.m_project != &project);
		StockPileProject& project2 = *static_cast<StockPileProject*>(dwarf2.m_project);
		REQUIRE(project2.hasCandidate(dwarf2));
		// Project2 reserves.
		simulation.doStep();
		REQUIRE(project2.reservationsComplete());
		REQUIRE(project2.getWorkers().contains(&dwarf2));
		std::function<bool()> predicate = [&](){ return chunk1.m_location == &stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		predicate = [&](){ return chunk2.m_location == &stockpileLocation2; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("Team haul")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, gold);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation1 = area.getBlock(5, 5, 1);
		Block& stockpileLocation2 = area.getBlock(5, 6, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.createItemGeneric(chunk, gold, 1u);
		chunk1.setLocation(chunkLocation);
		REQUIRE(!dwarf1.m_canPickup.maximumNumberWhichCanBeCarriedWithMinimumSpeed(chunk1, Config::minimumHaulSpeedInital));
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		Actor& dwarf2 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.getBlock(1, 2, 1));
		dwarf2.setFaction(&faction);
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(dwarf1));
		REQUIRE(objectiveType.canBeAssigned(dwarf2));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Add both actors as candidates.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		Project& project = *dwarf1.m_project;
		REQUIRE(project.hasCandidate(dwarf1));
		REQUIRE(project.hasCandidate(dwarf2));
		// Reserve target and destination.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		REQUIRE(dwarf2.m_project != nullptr);
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1).haulSubproject;
		REQUIRE(&haulSubproject == project.getProjectWorkerFor(dwarf2).haulSubproject);
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Team);
		std::function<bool()> predicate = [&](){ return chunk1.m_location == &stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("path to item is blocked")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 3, 1), area.getBlock(8, 3, 1), wood);
		Block& gateway = area.getBlock(9, 3, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		Project& project = *dwarf1.m_project;
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject);
		// Path to chunk.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination());
		REQUIRE(dwarf1.m_canMove.getDestination()->isAdjacentTo(chunk1));
		auto path = dwarf1.m_canMove.getPath();
		REQUIRE(std::ranges::find(path, &gateway) != path.end());
		gateway.setSolid(wood);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(!chunk1.m_reservable.isFullyReserved(&faction));
	}
	SUBCASE("path to stockpile is blocked")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 5, 1), area.getBlock(8, 5, 1), wood);
		Block& gateway = area.getBlock(9, 5, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 8, 1);
		Block& chunkLocation = area.getBlock(8, 1, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		Project& project = *dwarf1.m_project;
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject);
		// Path to chunk.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, chunkLocation);
		REQUIRE(dwarf1.m_canPickup.isCarrying(chunk1));
		// Path to stockpile.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "stockpile");
		REQUIRE(stockpileLocation.isAdjacentToIncludingCornersAndEdges(*dwarf1.m_canMove.getDestination()));
		gateway.setSolid(wood);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, gateway);
		REQUIRE(dwarf1.m_canPickup.isCarrying(chunk1));
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(!dwarf1.m_canPickup.isCarrying(chunk1));
		REQUIRE(!chunk1.m_reservable.isFullyReserved(&faction));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("path to haul tool is blocked")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 6, 1), area.getBlock(8, 6, 1), wood);
		Block& gateway = area.getBlock(9, 6, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, gold);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 5, 1);
		Block& cartLocation = area.getBlock(8, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, gold, 1u);
		chunk1.setLocation(chunkLocation);
		Item& cart1 = simulation.createItemNongeneric(cart, wood, 30u, 0);
		cart1.setLocation(cartLocation);
		area.m_hasHaulTools.registerHaulTool(cart1);
		REQUIRE(!dwarf1.m_canPickup.maximumNumberWhichCanBeCarriedWithMinimumSpeed(chunk1, Config::minimumHaulSpeedInital));
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Add candidate.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		Project& project = *dwarf1.m_project;
		REQUIRE(project.hasCandidate(dwarf1));
		// Reserve target and destination.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1).haulSubproject;
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Cart);
		// Path to cart.
		simulation.doStep();
		REQUIRE(cart1.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
		gateway.setSolid(wood);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, gateway);
		// Cannot detour, cancel subproject.
		simulation.doStep();
		//TODO: Project should either reset or cancel subproject ranther then canceling the whole thing.
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("haul tool destroyed")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(boulder, peridotite);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 5, 1);
		Block& cartLocation = area.getBlock(8, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& cargo = simulation.createItemGeneric(boulder, peridotite, 1u);
		cargo.setLocation(chunkLocation);
		Item& cart1 = simulation.createItemNongeneric(cart, wood, 30u, 0);
		cart1.setLocation(cartLocation);
		area.m_hasHaulTools.registerHaulTool(cart1);
		REQUIRE(!dwarf1.m_canPickup.canPickupAny(cargo));
		area.m_hasStockPiles.at(faction).addItem(cargo);
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Add candidate.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		Project& project = *dwarf1.m_project;
		REQUIRE(project.hasCandidate(dwarf1));
		// Reserve target and destination.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1).haulSubproject;
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Cart);
		cart1.destroy();
		REQUIRE(dwarf1.m_project->getWorkers().at(&dwarf1).haulSubproject == nullptr);
		REQUIRE(project.getHaulRetries() == 0);
		// Fast forward until haul project retry event spawns the haul retry threaded task.
		simulation.fastForward(Config::stepsFrequencyToLookForHaulSubprojects);
		REQUIRE(project.hasTryToHaulThreadedTask());
		simulation.doStep();
		REQUIRE(project.getHaulRetries() == 1);
		while(project.getHaulRetries() != Config::projectTryToMakeSubprojectRetriesBeforeProjectDelay - 1u)
		{
			simulation.fastForward(Config::stepsFrequencyToLookForHaulSubprojects);
			simulation.doStep();
		}
		simulation.fastForward(Config::stepsFrequencyToLookForHaulSubprojects);
		simulation.doStep();
		// Project has exhausted it's retry attempts.
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!cargo.m_reservable.isFullyReserved(&faction));
		REQUIRE(!cargo.m_canBeStockPiled.contains(faction));
		// Dwarf1 tries to find another stockpile job but cannot.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("item destroyed")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		chunk1.destroy();
		REQUIRE(dwarf1.m_project == nullptr);
		// Cannot find alternative stockpile project.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("stockpile undesignated by player")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		// Reserve required.
		simulation.doStep();
		REQUIRE(dwarf1.m_project->reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		stockpile.destroy();
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!objectiveType.canBeAssigned(dwarf1));
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk1) == nullptr);
		// Cannot find alternative stockpile project.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("destination set solid")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		stockpileLocation.setSolid(wood);
		REQUIRE(dwarf1.m_project == nullptr);
		// Cannot find alternative stockpile project.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("delay by player")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, area.getBlock(5, 9, 1));
		dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithProjectsCount() == 0);
	}
	SUBCASE("actor dies")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.getBlock(5, 5, 1);
		Block& chunkLocation = area.getBlock(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		dwarf1.die(CauseOfDeath::temperature);
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk1));
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithProjectsCount() == 0);
	}
	SUBCASE("some but not all reserved item from stack area destroyed")
	{
		//TODO
	}
}
