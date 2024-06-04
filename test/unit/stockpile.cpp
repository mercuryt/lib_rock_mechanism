#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/item.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/stockpile.h"
#include "../../engine/materialType.h"
#include "../../engine/project.h"
#include "../../engine/config.h"
#include "../../engine/haul.h"
#include "../../engine/objectives/goTo.h"
#include "types.h"
#include <functional>
#include <memory>
TEST_CASE("stockpile")
{
	const MaterialType& wood = MaterialType::byName("poplar wood");
	const MaterialType& marble = MaterialType::byName("marble");
	const MaterialType& gold = MaterialType::byName("gold");
	const MaterialType& peridotite = MaterialType::byName("peridotite");
	const MaterialType& sand = MaterialType::byName("sand");
	const ItemType& chunk = ItemType::byName("chunk");
	const ItemType& cart = ItemType::byName("cart");
	const ItemType& boulder = ItemType::byName("boulder");
	const ItemType& pile = ItemType::byName("pile");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.registerFaction(faction);
	area.m_hasStocks.addFaction(faction);
	Actor& dwarf1 = simulation.m_hasActors->createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex({1, 1, 1}),
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
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
		REQUIRE(dwarf1.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, chunkLocation);
		REQUIRE(dwarf1.m_canPickup.isCarrying(chunk1));
		std::function<bool()> predicate = [&](){ return chunk1.m_location == stockpileLocation; };
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
		BlockIndex stockpileLocation1 = blocks.getIndex({5, 5, 1});
		BlockIndex stockpileLocation2 = blocks.getIndex({5, 6, 1});
		BlockIndex chunkLocation1 = blocks.getIndex({1, 8, 1});
		BlockIndex chunkLocation2 = blocks.getIndex({9, 8, 1});
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
		Item& chunk2 = simulation.m_hasItems->createItemGeneric(chunk, marble, 1u);
		chunk1.setLocation(chunkLocation1);
		chunk2.setLocation(chunkLocation2);
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk1) == &stockpile);
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk2) == &stockpile);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		area.m_hasStockPiles.at(faction).addItem(chunk2);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinations().size() == 2);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinationsByStockPile().size() == 1);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		std::function<bool()> predicate = [&](){ return chunk1.m_location == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(objectiveType.canBeAssigned(dwarf1));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "stockpile");
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinations().size() == 1);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinationsByStockPile().size() == 1);
		// Find the second item and stockpile slot, create project and add dwarf1 as candidate.
		simulation.doStep();
		REQUIRE(dwarf1.m_project->getLocation() == stockpileLocation2);
		predicate = [&](){ return chunk2.m_location == stockpileLocation2 && dwarf1.m_project == nullptr; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(!objectiveType.canBeAssigned(dwarf1));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("Two workers haul two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation1 = blocks.getIndex({3, 3, 1});
		BlockIndex stockpileLocation2 = blocks.getIndex({3, 4, 1});
		BlockIndex chunkLocation1 = blocks.getIndex({1, 8, 1});
		BlockIndex chunkLocation2 = blocks.getIndex({9, 9, 1});
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, marble, 1u);
		Item& chunk2 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
		chunk1.setLocation(chunkLocation1);
		chunk2.setLocation(chunkLocation2);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		area.m_hasStockPiles.at(faction).addItem(chunk2);
		Actor& dwarf2 = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf"), blocks.getIndex({1, 2, 1}));
		dwarf2.setFaction(&faction);
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(dwarf2));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Both dwarves become candidates for chunk1.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		StockPileProject& project = *static_cast<StockPileProject*>(dwarf1.m_project);
		REQUIRE(project.getItem() == chunk1);
		REQUIRE(project.hasCandidate(dwarf1));
		REQUIRE(project.hasCandidate(dwarf2));
		REQUIRE(static_cast<StockPileObjective&>(dwarf1.m_hasObjectives.getCurrent()).m_project != nullptr);
		// Chunk1 project reserves.
		simulation.doStep();
		Actor* project1Worker = nullptr;
		Actor* project2Worker = nullptr;
		if(project.getWorkers().contains(&dwarf1))
		{
			project1Worker = &dwarf1;
			project2Worker = &dwarf2;
		}
		else
		{
			REQUIRE(project.getWorkers().contains(&dwarf2));
			project1Worker = &dwarf2;
			project2Worker = &dwarf1;
		}
		REQUIRE(!project.getWorkers().contains(project2Worker));
		REQUIRE(project.reservationsComplete());
		// Project1 creates a haulsubproject, Project2 is created.
		simulation.doStep();
		REQUIRE(project.getWorkers().contains(project1Worker));
		REQUIRE(project.getProjectWorkerFor(*project1Worker).haulSubproject);
		REQUIRE(static_cast<StockPileObjective&>(project1Worker->m_hasObjectives.getCurrent()).m_project);
		REQUIRE(!project.getWorkers().contains(project2Worker));
		REQUIRE(project2Worker->m_project);
		REQUIRE(project2Worker->m_hasObjectives.getCurrent().name() == "stockpile");
		StockPileProject& project2 = *static_cast<StockPileProject*>(project2Worker->m_project);
		REQUIRE(project2 != project);
		REQUIRE(project2.hasCandidate(*project2Worker));
		REQUIRE(project2.getItem() == chunk2);
		// Project1 paths to chunk1, Project2 reserves chunk2
		simulation.doStep();
		REQUIRE(project1Worker->m_canMove.getDestination());
		REQUIRE(chunk1.isAdjacentTo(project1Worker->m_canMove.getDestination()));
		REQUIRE(project2Worker->m_project);
		REQUIRE(project2Worker->m_project != &project);
		REQUIRE(project2.getWorkers().contains(project2Worker));
		REQUIRE(project2.reservationsComplete());
		// Project2 creates haul subproject.
		simulation.doStep();
		REQUIRE(project2.getWorkers().contains(project2Worker));
		REQUIRE(project2.getProjectWorkerFor(*project2Worker).haulSubproject);
		// Project2 paths.
		simulation.doStep();
		REQUIRE(project2Worker->m_canMove.getDestination());
		std::function<bool()> predicate = [&](){ return chunk1.m_location == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		predicate = [&](){ return chunk2.m_location == stockpileLocation2; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(project1Worker->m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(project2Worker->m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("Team haul")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, gold);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation1 = blocks.getIndex({5, 5, 1});
		BlockIndex stockpileLocation2 = blocks.getIndex({5, 6, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, gold, 1u);
		chunk1.setLocation(chunkLocation);
		REQUIRE(!dwarf1.m_canPickup.maximumNumberWhichCanBeCarriedWithMinimumSpeed(chunk1, Config::minimumHaulSpeedInital));
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		Actor& dwarf2 = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf"), blocks.getIndex({1, 2, 1}));
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
		std::function<bool()> predicate = [&](){ return chunk1.m_location == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("haul generic")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex cargoOrigin = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& cargo = simulation.m_hasItems->createItemGeneric(pile, sand, 100u);
		cargo.setLocation(cargoOrigin);
		uint32_t quantity = dwarf1.m_canPickup.maximumNumberWhichCanBeCarriedWithMinimumSpeed(cargo, Config::minimumHaulSpeedInital);
		REQUIRE(quantity == 48);
		area.m_hasStockPiles.at(faction).addItem(cargo);
		REQUIRE(objectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		REQUIRE(area.m_hasStockPiles.at(faction).getHaulableItemForAt(dwarf1, cargoOrigin));
		// Find item to stockpile, make project.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr);
		Project& project = *dwarf1.m_project;
		// Reserve required.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		// Path to cargo.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, cargoOrigin);
		REQUIRE(dwarf1.m_canPickup.isCarryingGeneric(pile, sand, 48u));
		REQUIRE(blocks.shape_contains(cargoOrigin, cargo));
		REQUIRE(cargo.getQuantity() == 52);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, stockpileLocation);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "stockpile");
		auto predicate = [&]{ return !blocks.item_empty(stockpileLocation); };
		simulation.fastForwardUntillPredicate(predicate);
		bool shouldBeTrue = blocks.stockpile_isAvalible(stockpileLocation, faction);
		REQUIRE(shouldBeTrue);
		REQUIRE(blocks.item_getCount(stockpileLocation, pile, sand) == 48);
		REQUIRE(!dwarf1.m_canPickup.exists());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "stockpile");
		StockPileObjective& objective = static_cast<StockPileObjective&>(dwarf1.m_hasObjectives.getCurrent());
		REQUIRE(objective.m_threadedTask.exists());
		auto predicate2 = [&]{ return !dwarf1.m_canMove.getPath().empty(); };
		simulation.fastForwardUntillPredicate(predicate2);
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, cargoOrigin);
		REQUIRE(dwarf1.m_canPickup.isCarryingGeneric(pile, sand, 48u));
		REQUIRE(cargo.getQuantity() == 4);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, stockpileLocation);
		auto predicate3 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 96; };
		simulation.fastForwardUntillPredicate(predicate3);
		REQUIRE(blocks.item_getCount(stockpileLocation, pile, sand) == 96);
		simulation.fastForwardUntillPredicate(predicate2);
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, cargoOrigin);
		REQUIRE(dwarf1.m_canPickup.isCarryingGeneric(pile, sand, 4u));
		REQUIRE(blocks.item_getCount(cargoOrigin, pile, sand) == 0);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, stockpileLocation);
		auto predicate4 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 100; };
		simulation.fastForwardUntillPredicate(predicate4);
		REQUIRE(!dwarf1.m_canPickup.exists());
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("haul generic two workers")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex cargoOrigin = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& cargo = simulation.m_hasItems->createItemGeneric(pile, sand, 100u);
		cargo.setLocation(cargoOrigin);
		area.m_hasStockPiles.at(faction).addItem(cargo);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		Actor& dwarf2 = simulation.m_hasActors->createActor(AnimalSpecies::byName("dwarf"), blocks.getIndex({1, 2, 1}));
		dwarf2.setFaction(&faction);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// Both workers find hauling task.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		REQUIRE(dwarf2.m_project);
		// One worker is selected when the project makes reservations. This is the first worker.
		simulation.doStep();
		// The second worker finds the task again, the first selects a haul strategy.
		simulation.doStep();
		// The second worker's project makes reservations, the first paths to cargoOrigin.
		simulation.doStep();
		// The second worker selects a haul strategy.
		simulation.doStep();
		// the second worker paths to cargoOrigin.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf2, cargoOrigin);
		REQUIRE(dwarf2.m_canPickup.exists());
		auto predicate1 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand); };
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "stockpile");
		simulation.fastForwardUntillPredicate(predicate1);
		Quantity firstDeliveryQuantity = blocks.item_getCount(stockpileLocation, pile, sand);
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "stockpile");
		auto predicate2 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) > firstDeliveryQuantity; };
		simulation.fastForwardUntillPredicate(predicate2);
		if(dwarf2.getActionDescription() != L"stockpile")
			REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "stockpile");
		else
			REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "stockpile");
		auto predicate3 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 100; };
		simulation.fastForwardUntillPredicate(predicate3);
		REQUIRE(!dwarf1.m_project);
		REQUIRE(!dwarf2.m_project);
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "stockpile");
	}
	SUBCASE("haul generic two stockpile blocks")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation1 = blocks.getIndex({5, 5, 1});
		BlockIndex stockpileLocation2 = blocks.getIndex({6, 5, 1});
		BlockIndex cargoOrigin1 = blocks.getIndex({1, 8, 1});
		BlockIndex cargoOrigin2 = blocks.getIndex({2, 8, 1});
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		Item& cargo1 = simulation.m_hasItems->createItemGeneric(pile, sand, 100u);
		Item& cargo2 = simulation.m_hasItems->createItemGeneric(pile, sand, 100u);
		cargo1.setLocation(cargoOrigin1);
		cargo2.setLocation(cargoOrigin2);
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, cargoOrigin1);
		REQUIRE(blocks.item_getCount(cargoOrigin1, pile, sand) < 100);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, stockpileLocation1);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, cargoOrigin1);
		REQUIRE(dwarf1.m_project);
		REQUIRE(dwarf1.m_project->getLocation() == stockpileLocation1);
	}
	SUBCASE("haul generic stockpile is adjacent to cargo")
	{
		BlockIndex pileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex stockpileLocation = blocks.getIndex({6, 5, 1});
		Item& cargo = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo.setLocation(pileLocation);
		area.m_hasStockPiles.at(faction).addItem(cargo);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		// One step to generate project.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		// One step to reserve.
		simulation.doStep();
		REQUIRE(dwarf1.m_project->reservationsComplete());
		// Since the only required item is already adjacent to the project site deleveries are also complete.
		REQUIRE(dwarf1.m_project->deliveriesComplete());
		// Path to location.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, stockpileLocation);
		REQUIRE(dwarf1.m_project->finishEventExists());
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 15; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("haul generic stockpile is adjacent to two cargo piles")
	{
		BlockIndex pileLocation1 = blocks.getIndex({5, 5, 1});
		BlockIndex stockpileLocation = blocks.getIndex({6, 5, 1});
		BlockIndex pileLocation2 = blocks.getIndex({7, 5, 1});
		Item& cargo1 = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo1.setLocation(pileLocation1);
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		Item& cargo2 = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo2.setLocation(pileLocation2);
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("haul generic consolidate adjacent")
	{
		BlockIndex pileLocation1 = blocks.getIndex({5, 5, 1});
		BlockIndex stockpileLocation = blocks.getIndex({6, 5, 1});
		BlockIndex pileLocation2 = blocks.getIndex({7, 5, 1});
		Item& cargo1 = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo1.setLocation(pileLocation1);
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		Item& cargo2 = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo2.setLocation(pileLocation2);
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		Item& cargo3 = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo3.setLocation(stockpileLocation);
		area.m_hasStockPiles.at(faction).addItem(cargo3);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
		REQUIRE(dwarf1.getActionDescription() == L"stockpile");
		REQUIRE(!dwarf1.m_project);
		REQUIRE(static_cast<StockPileObjective&>(dwarf1.m_hasObjectives.getCurrent()).m_threadedTask.exists());
		// Step to create project.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		// Step to reserve.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		REQUIRE(dwarf1.m_project->reservationsComplete());
		REQUIRE(dwarf1.m_project->deliveriesComplete());
		REQUIRE(dwarf1.m_project->finishEventExists());
		auto condition2 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 45; };
		simulation.fastForwardUntillPredicate(condition2);
	}
	SUBCASE("tile which worker stands on for work phase of project contains item of the same generic type")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({8, 1, 1}), blocks.getIndex({9, 1, 1}), marble);
		BlockIndex stockpileLocation = blocks.getIndex({9, 0, 1});
		BlockIndex pileLocation1 = blocks.getIndex({8, 0, 1});
		BlockIndex pileLocation2 = blocks.getIndex({7, 0, 1});
		Item& cargo1 = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo1.setLocation(pileLocation1);
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		Item& cargo2 = simulation.m_hasItems->createItemGeneric(pile, sand, 15);
		cargo2.setLocation(pileLocation2);
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 100);
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("path to item is blocked")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 3, 1}), blocks.getIndex({8, 3, 1}), wood);
		BlockIndex gateway = blocks.getIndex({9, 3, 1});
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
		REQUIRE(chunk1.isAdjacentTo(dwarf1.m_canMove.getDestination()));
		auto path = dwarf1.m_canMove.getPath();
		REQUIRE(std::ranges::find(path, gateway) != path.end());
		blocks.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "stockpile");
		REQUIRE(!chunk1.m_reservable.isFullyReserved(&faction));
	}
	SUBCASE("path to stockpile is blocked")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 5, 1}), blocks.getIndex({8, 5, 1}), wood);
		BlockIndex gateway = blocks.getIndex({9, 5, 1});
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex({5, 8, 1});
		BlockIndex chunkLocation = blocks.getIndex({8, 1, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(stockpileLocation, dwarf1.m_canMove.getDestination()));
		blocks.solid_set(gateway, wood, false);
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
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 6, 1}), blocks.getIndex({8, 6, 1}), wood);
		BlockIndex gateway = blocks.getIndex({9, 6, 1});
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, gold);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 5, 1});
		BlockIndex cartLocation = blocks.getIndex({8, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, gold, 1u);
		chunk1.setLocation(chunkLocation);
		Item& cart1 = simulation.m_hasItems->createItemNongeneric(cart, wood, 30u, 0);
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
		REQUIRE(cart1.isAdjacentTo(dwarf1.m_canMove.getDestination()));
		blocks.solid_set(gateway, wood, false);
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
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 5, 1});
		BlockIndex cartLocation = blocks.getIndex({8, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& cargo = simulation.m_hasItems->createItemGeneric(boulder, peridotite, 1u);
		cargo.setLocation(chunkLocation);
		Item& cart1 = simulation.m_hasItems->createItemNongeneric(cart, wood, 30u, 0);
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
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
		blocks.solid_set(stockpileLocation, wood, false);
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
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, blocks.getIndex({5, 9, 1}));
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
		BlockIndex stockpileLocation = blocks.getIndex({5, 5, 1});
		BlockIndex chunkLocation = blocks.getIndex({1, 8, 1});
		stockpile.addBlock(stockpileLocation);
		Item& chunk1 = simulation.m_hasItems->createItemGeneric(chunk, wood, 1u);
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
