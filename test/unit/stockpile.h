#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/item.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/stockpile.h"
#include "../../src/materialType.h"
#include "../../src/project.h"
#include "haul.h"
#include <functional>
TEST_CASE("stockpile")
{
	const MaterialType& wood = MaterialType::byName("poplar wood");
	const MaterialType& marble = MaterialType::byName("marble");
	const MaterialType& lead = MaterialType::byName("lead");
	const ItemType& chunk = ItemType::byName("chunk");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.addFaction(faction);
	Actor& dwarf1 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.m_blocks[1][1][1]);
	dwarf1.setFaction(&faction);
	area.m_hasActors.add(dwarf1);
	StockPileObjectiveType objectiveType;
	SUBCASE("basic")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation = area.m_blocks[5][5][1];
		Block& chunkLocation = area.m_blocks[1][8][1];
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.createItem(chunk, wood, 1u);
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
	SUBCASE("Haul two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation1 = area.m_blocks[5][5][1];
		Block& stockpileLocation2 = area.m_blocks[5][6][1];
		Block& chunkLocation1 = area.m_blocks[1][8][1];
		Block& chunkLocation2 = area.m_blocks[9][8][1];
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.createItem(chunk, wood, 1u);
		Item& chunk2 = simulation.createItem(chunk, marble, 1u);
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
		Block& stockpileLocation1 = area.m_blocks[5][5][1];
		Block& stockpileLocation2 = area.m_blocks[5][6][1];
		Block& chunkLocation1 = area.m_blocks[1][8][1];
		Block& chunkLocation2 = area.m_blocks[9][8][1];
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.createItem(chunk, marble, 1u);
		Item& chunk2 = simulation.createItem(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation1);
		chunk2.setLocation(chunkLocation2);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		area.m_hasStockPiles.at(faction).addItem(chunk2);
		Actor& dwarf2 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.m_blocks[1][2][1]);
		dwarf2.setFaction(&faction);
		area.m_hasActors.add(dwarf2);
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
		queries.emplace_back(chunk, lead);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		Block& stockpileLocation1 = area.m_blocks[5][5][1];
		Block& stockpileLocation2 = area.m_blocks[5][6][1];
		Block& chunkLocation = area.m_blocks[1][8][1];
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.createItem(chunk, lead, 1u);
		chunk1.setLocation(chunkLocation);
		REQUIRE(!dwarf1.m_canPickup.canPickupAny(chunk1));
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		Actor& dwarf2 = simulation.createActor(AnimalSpecies::byName("dwarf"), area.m_blocks[1][2][1]);
		dwarf2.setFaction(&faction);
		area.m_hasActors.add(dwarf2);
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
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1).haulSubproject;
		REQUIRE(&haulSubproject == project.getProjectWorkerFor(dwarf2).haulSubproject);
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Team);
		std::function<bool()> predicate = [&](){ return chunk1.m_location == &stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "stockpile");
	}
}
