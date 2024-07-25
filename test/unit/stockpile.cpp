#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/area.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/stockpile.h"
#include "../../engine/materialType.h"
#include "../../engine/itemType.h"
#include "../../engine/project.h"
#include "../../engine/config.h"
#include "../../engine/haul.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/stockpile.h"
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
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Faction faction(L"tower of power");
	area.m_hasStockPiles.registerFaction(faction);
	area.m_hasStocks.addFaction(faction);
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName("dwarf"), 
		.location=blocks.getIndex(1, 1, 1),
		.faction=&faction,
		.hasCloths=false,
		.hasSidearm=false,
	});
	StockPileObjectiveType objectiveType;
	SUBCASE("basic")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=1u});
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		REQUIRE(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		REQUIRE(area.m_hasStockPiles.at(faction).getHaulableItemForAt(dwarf1, chunkLocation));
		// Find item to stockpile, make project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) != nullptr);
		Project& project = *actors.project_get(dwarf1);
		REQUIRE(project.hasCandidate(dwarf1));
		// Reserve required.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		// Path to chunk.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		std::function<bool()> predicate = [&](){ return items.getLocation(chunk1) == stockpileLocation; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		REQUIRE(!objectiveType.canBeAssigned(area, dwarf1));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("one worker hauls two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation1 = blocks.getIndex(5, 5, 1);
		BlockIndex stockpileLocation2 = blocks.getIndex(5, 6, 1);
		BlockIndex chunkLocation1 = blocks.getIndex(1, 8, 1);
		BlockIndex chunkLocation2 = blocks.getIndex(9, 8, 1);
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation1, .quantity=1u});
		ItemIndex chunk2 = items.create({.itemType=chunk, .materialType=marble, .location=chunkLocation2, .quantity=1u});
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk1) == &stockpile);
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk2) == &stockpile);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		area.m_hasStockPiles.at(faction).addItem(chunk2);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinations().size() == 2);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinationsByStockPile().size() == 1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		std::function<bool()> predicate = [&](){ return items.getLocation(chunk1) == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(objectiveType.canBeAssigned(area, dwarf1));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "stockpile");
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinations().size() == 1);
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithDestinationsByStockPile().size() == 1);
		// Find the second item and stockpile slot, create project and add dwarf1 as candidate.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1)->getLocation() == stockpileLocation2);
		predicate = [&](){ return items.getLocation(chunk2) == stockpileLocation2 && actors.project_get(dwarf1) == nullptr; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(!objectiveType.canBeAssigned(area, dwarf1));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("Two workers haul two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation1 = blocks.getIndex(3, 3, 1);
		BlockIndex stockpileLocation2 = blocks.getIndex(3, 4, 1);
		BlockIndex chunkLocation1 = blocks.getIndex(1, 8, 1);
		BlockIndex chunkLocation2 = blocks.getIndex(9, 9, 1);
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=marble, .location=chunkLocation1, .quantity=1u});
		ItemIndex chunk2 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation2, .quantity=1u});
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		area.m_hasStockPiles.at(faction).addItem(chunk2);
		ActorIndex dwarf2 = actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=blocks.getIndex(1, 2, 1),
			.faction=&faction,
		});
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(area, dwarf2));
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		actors.objective_setPriority(dwarf2, objectiveType, 100);
		// Both dwarves become candidates for chunk1.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) != nullptr);
		StockPileProject& project = *static_cast<StockPileProject*>(actors.project_get(dwarf1));
		REQUIRE(project.getItem() == chunk1);
		REQUIRE(project.hasCandidate(dwarf1));
		REQUIRE(project.hasCandidate(dwarf2));
		REQUIRE(actors.objective_getCurrent<StockPileObjective>(dwarf1).m_project != nullptr);
		// Chunk1 project reserves.
		simulation.doStep();
		ActorIndex project1Worker.clear();
		ActorIndex project2Worker.clear();
		if(project.getWorkers().contains(dwarf1))
		{
			project1Worker = dwarf1;
			project2Worker = dwarf2;
		}
		else
		{
			REQUIRE(project.getWorkers().contains(dwarf2));
			project1Worker = dwarf2;
			project2Worker = dwarf1;
		}
		REQUIRE(!project.getWorkers().contains(project2Worker));
		REQUIRE(project.reservationsComplete());
		// Project1 creates a haulsubproject, Project2 is created.
		simulation.doStep();
		REQUIRE(project.getWorkers().contains(project1Worker));
		REQUIRE(project.getProjectWorkerFor(project1Worker).haulSubproject);
		REQUIRE(actors.objective_getCurrent<StockPileObjective>(project1Worker).m_project);
		REQUIRE(!project.getWorkers().contains(project2Worker));
		REQUIRE(actors.project_exists(project2Worker));
		REQUIRE(actors.objective_getCurrentName(project2Worker) == "stockpile");
		StockPileProject& project2 = *static_cast<StockPileProject*>(actors.project_get(project2Worker));
		REQUIRE(project2 != project);
		REQUIRE(project2.hasCandidate(project2Worker));
		REQUIRE(project2.getItem() == chunk2);
		// Project1 paths to chunk1, Project2 reserves chunk2
		simulation.doStep();
		REQUIRE(actors.move_getDestination(project1Worker).exists());
		REQUIRE(items.isAdjacentToLocation(chunk1, actors.move_getDestination(project1Worker)));
		REQUIRE(actors.project_exists(project2Worker));
		REQUIRE(actors.project_get(project2Worker) != &project);
		REQUIRE(project2.getWorkers().contains(project2Worker));
		REQUIRE(project2.reservationsComplete());
		// Project2 creates haul subproject.
		simulation.doStep();
		REQUIRE(project2.getWorkers().contains(project2Worker));
		REQUIRE(project2.getProjectWorkerFor(project2Worker).haulSubproject);
		// Project2 paths.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(project2Worker).exists());
		std::function<bool()> predicate = [&](){ return items.getLocation(chunk1) == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		predicate = [&](){ return actors.getLocation(chunk2) == stockpileLocation2; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(actors.objective_getCurrentName(project1Worker) != "stockpile");
		REQUIRE(actors.objective_getCurrentName(project2Worker) != "stockpile");
	}
	SUBCASE("Team haul")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, gold);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation1 = blocks.getIndex(5, 5, 1);
		BlockIndex stockpileLocation2 = blocks.getIndex(5, 6, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=1u});
		REQUIRE(!actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, chunk1, Config::minimumHaulSpeedInital));
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		ActorIndex dwarf2 = actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=blocks.getIndex(1, 2, 1),
			.faction=&faction,
		});
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(area, dwarf1));
		REQUIRE(objectiveType.canBeAssigned(area, dwarf2));
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		actors.objective_setPriority(dwarf2, objectiveType, 100);
		// Add both actors as candidates.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) != nullptr);
		Project& project = *actors.project_get(dwarf1);
		REQUIRE(project.hasCandidate(dwarf1));
		REQUIRE(project.hasCandidate(dwarf2));
		// Reserve target and destination.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		REQUIRE(!actors.project_exists(dwarf2));
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1).haulSubproject;
		REQUIRE(&haulSubproject == project.getProjectWorkerFor(dwarf2).haulSubproject);
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Team);
		std::function<bool()> predicate = [&](){ return items.getLocation(chunk1) == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
		REQUIRE(actors.objective_getCurrentName(dwarf2) != "stockpile");
	}
	SUBCASE("haul generic")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex cargoOrigin = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex cargo = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin, .quantity=100u});
		uint32_t quantity = actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, cargo, Config::minimumHaulSpeedInital);
		REQUIRE(quantity == 48);
		area.m_hasStockPiles.at(faction).addItem(cargo);
		REQUIRE(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		REQUIRE(area.m_hasStockPiles.at(faction).getHaulableItemForAt(dwarf1, cargoOrigin));
		// Find item to stockpile, make project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) != nullptr);
		Project& project = *actors.project_get(dwarf1);
		// Reserve required.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		// Path to cargo.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoOrigin);
		REQUIRE(actors.canPickUp_isCarryingItemGeneric(dwarf1, pile, sand, 48u));
		REQUIRE(blocks.item_contains(cargoOrigin, cargo));
		REQUIRE(items.getQuantity(cargo) == 52);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, stockpileLocation);
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "stockpile");
		auto predicate = [&]{ return !blocks.item_empty(stockpileLocation); };
		simulation.fastForwardUntillPredicate(predicate);
		bool shouldBeTrue = blocks.stockpile_isAvalible(stockpileLocation, faction);
		REQUIRE(shouldBeTrue);
		REQUIRE(blocks.item_getCount(stockpileLocation, pile, sand) == 48);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "stockpile");
		REQUIRE(actors.move_hasPathRequest(dwarf1));
		auto predicate2 = [&]{ return !actors.move_getPath(dwarf1).empty(); };
		simulation.fastForwardUntillPredicate(predicate2);
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoOrigin);
		REQUIRE(actors.canPickUp_isCarryingItemGeneric(dwarf1, pile, sand, 48u));
		REQUIRE(items.getQuantity(cargo) == 4);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, stockpileLocation);
		auto predicate3 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 96; };
		simulation.fastForwardUntillPredicate(predicate3);
		REQUIRE(blocks.item_getCount(stockpileLocation, pile, sand) == 96);
		simulation.fastForwardUntillPredicate(predicate2);
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoOrigin);
		REQUIRE(actors.canPickUp_isCarryingItemGeneric(dwarf1, pile, sand, 4u));
		REQUIRE(blocks.item_getCount(cargoOrigin, pile, sand) == 0);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, stockpileLocation);
		auto predicate4 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 100; };
		simulation.fastForwardUntillPredicate(predicate4);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("haul generic two workers")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex cargoOrigin = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex cargo = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin, .quantity=100u});
		area.m_hasStockPiles.at(faction).addItem(cargo);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		ActorIndex dwarf2 = actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=blocks.getIndex(1, 2, 1),
			.faction=&faction,
		});
		actors.objective_setPriority(dwarf2, objectiveType, 100);
		// Both workers find hauling task.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		REQUIRE(actors.project_exists(dwarf2));
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
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf2, cargoOrigin);
		REQUIRE(actors.canPickUp_exists(dwarf2));
		auto predicate1 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand); };
		REQUIRE(actors.objective_getCurrentName(dwarf2) == "stockpile");
		simulation.fastForwardUntillPredicate(predicate1);
		Quantity firstDeliveryQuantity = blocks.item_getCount(stockpileLocation, pile, sand);
		REQUIRE(actors.objective_getCurrentName(dwarf2) == "stockpile");
		auto predicate2 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) > firstDeliveryQuantity; };
		simulation.fastForwardUntillPredicate(predicate2);
		if(actors.getActionDescription(dwarf2) != L"stockpile")
			REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
		else
			REQUIRE(actors.objective_getCurrentName(dwarf2) == "stockpile");
		auto predicate3 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 100; };
		simulation.fastForwardUntillPredicate(predicate3);
		REQUIRE(!actors.project_get(dwarf1));
		REQUIRE(!actors.project_exists(dwarf2));
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
		REQUIRE(actors.objective_getCurrentName(dwarf2) != "stockpile");
	}
	SUBCASE("haul generic two stockpile blocks")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation1 = blocks.getIndex(5, 5, 1);
		BlockIndex stockpileLocation2 = blocks.getIndex(6, 5, 1);
		BlockIndex cargoOrigin1 = blocks.getIndex(1, 8, 1);
		BlockIndex cargoOrigin2 = blocks.getIndex(2, 8, 1);
		stockpile.addBlock(stockpileLocation1);
		stockpile.addBlock(stockpileLocation2);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin1, .quantity=100u});
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin2, .quantity=100u});
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, cargoOrigin1);
		REQUIRE(blocks.item_getCount(cargoOrigin1, pile, sand) < 100);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, stockpileLocation1);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, cargoOrigin1);
		REQUIRE(actors.project_get(dwarf1));
		REQUIRE(actors.project_get(dwarf1)->getLocation() == stockpileLocation1);
	}
	SUBCASE("haul generic stockpile is adjacent to cargo")
	{
		BlockIndex pileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex stockpileLocation = blocks.getIndex(6, 5, 1);
		ItemIndex cargo = items.create({.itemType=pile, .materialType=sand, .location=pileLocation, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// One step to generate project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		// One step to reserve.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1)->reservationsComplete());
		// Since the only required item is already adjacent to the project site deleveries are also complete.
		REQUIRE(actors.project_get(dwarf1)->deliveriesComplete());
		// Path to location.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, stockpileLocation);
		REQUIRE(actors.project_get(dwarf1)->finishEventExists());
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 15; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("haul generic stockpile is adjacent to two cargo piles")
	{
		BlockIndex pileLocation1 = blocks.getIndex(5, 5, 1);
		BlockIndex stockpileLocation = blocks.getIndex(6, 5, 1);
		BlockIndex pileLocation2 = blocks.getIndex(7, 5, 1);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation1, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation2, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("haul generic consolidate adjacent")
	{
		BlockIndex pileLocation1 = blocks.getIndex(5, 5, 1);
		BlockIndex stockpileLocation = blocks.getIndex(6, 5, 1);
		BlockIndex pileLocation2 = blocks.getIndex(7, 5, 1);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation1, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation2, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		ItemIndex cargo3 = items.create({.itemType=pile, .materialType=sand, .location=stockpileLocation, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo3);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
		REQUIRE(actors.getActionDescription(dwarf1) == L"stockpile");
		REQUIRE(!actors.project_get(dwarf1));
		REQUIRE(actors.move_hasPathRequest(dwarf1));
		// Step to create project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		// Step to reserve.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		REQUIRE(actors.project_get(dwarf1)->reservationsComplete());
		REQUIRE(actors.project_get(dwarf1)->deliveriesComplete());
		REQUIRE(actors.project_get(dwarf1)->finishEventExists());
		auto condition2 = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 45; };
		simulation.fastForwardUntillPredicate(condition2);
	}
	SUBCASE("tile which worker stands on for work phase of project contains item of the same generic type")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex(8, 1, 1), blocks.getIndex({9, 1, 1}), marble);
		BlockIndex stockpileLocation = blocks.getIndex(9, 0, 1);
		BlockIndex pileLocation1 = blocks.getIndex(8, 0, 1);
		BlockIndex pileLocation2 = blocks.getIndex(7, 0, 1);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation1, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo1);
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation2, .quantity=15});
		area.m_hasStockPiles.at(faction).addItem(cargo2);
		std::vector<ItemQuery> queries;
		queries.emplace_back(pile, sand);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		stockpile.addBlock(stockpileLocation);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		auto condition = [&]{ return blocks.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("path to item is blocked")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex(0, 3, 1), blocks.getIndex({8, 3, 1}), wood);
		BlockIndex gateway = blocks.getIndex(9, 3, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=1u});
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		Project& project = *actors.project_get(dwarf1);
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject);
		// Path to chunk.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		REQUIRE(items.isAdjacentToActor(chunk1, actors.move_getDestination(dwarf1)));
		auto path = actors.move_getPath(dwarf1);
		REQUIRE(std::ranges::find(path, gateway) != path.end());
		blocks.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
		REQUIRE(!items.reservable_isFullyReserved(chunk1, faction));
	}
	SUBCASE("path to stockpile is blocked")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex(0, 5, 1), blocks.getIndex({8, 5, 1}), wood);
		BlockIndex gateway = blocks.getIndex(9, 5, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 8, 1);
		BlockIndex chunkLocation = blocks.getIndex(8, 1, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=1u});
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		Project& project = *actors.project_get(dwarf1);
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject);
		// Path to chunk.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		// Path to stockpile.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "stockpile");
		REQUIRE(blocks.isAdjacentToIncludingCornersAndEdges(stockpileLocation, actors.move_getDestination(dwarf1)));
		blocks.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(!actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		REQUIRE(!items.reservable_isFullyReserved(chunk1, faction));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("path to haul tool is blocked")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex(0, 6, 1), blocks.getIndex({8, 6, 1}), wood);
		BlockIndex gateway = blocks.getIndex(9, 6, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, gold);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 5, 1);
		BlockIndex cartLocation = blocks.getIndex(8, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=1u});
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=wood, .location=cartLocation, .quality=30, .percentWear=0});
		REQUIRE(!actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, chunk1, Config::minimumHaulSpeedInital));
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Add candidate.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) != nullptr);
		Project& project = *actors.project_get(dwarf1);
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
		REQUIRE(items.isAdjacentToLocation(cart1, actors.move_getDestination(dwarf1)));
		blocks.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour, cancel subproject.
		simulation.doStep();
		//TODO: Project should either reset or cancel subproject ranther then canceling the whole thing.
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("haul tool destroyed")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(boulder, peridotite);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 5, 1);
		BlockIndex cartLocation = blocks.getIndex(8, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex cargo = items.create({.itemType=boulder, .materialType=peridotite, .location=chunkLocation, .quantity=1});
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=wood, .location=cartLocation, .quality=30, .percentWear=0});
		REQUIRE(!actors.canPickUp_item(dwarf1, cargo));
		area.m_hasStockPiles.at(faction).addItem(cargo);
		StockPileObjectiveType objectiveType;
		REQUIRE(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Add candidate.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) != nullptr);
		Project& project = *actors.project_get(dwarf1);
		REQUIRE(project.hasCandidate(dwarf1));
		// Reserve target and destination.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1).haulSubproject;
		REQUIRE(haulSubproject.getHaulStrategy() == HaulStrategy::Cart);
		items.destroy(cart1);
		REQUIRE(actors.project_get(dwarf1)->getWorkers().at(dwarf1).haulSubproject == nullptr);
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
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		REQUIRE(!items.reservable_isFullyReserved(cargo, faction));
		REQUIRE(!items.stockpile_canBeStockPiled(cargo, faction));
		// Dwarf1 tries to find another stockpile job but cannot.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("item destroyed")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .quantity=1u});
		items.setLocation(chunk1, chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		items.destroy(chunk1);
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		// Cannot find alternative stockpile project.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("stockpile undesignated by player")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .quantity=1u});
		items.setLocation(chunk1, chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) != nullptr);
		// Reserve required.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1)->reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		stockpile.destroy();
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		REQUIRE(!objectiveType.canBeAssigned(area, dwarf1));
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk1) == nullptr);
		// Cannot find alternative stockpile project.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("destination set solid")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .quantity=1u});
		items.setLocation(chunk1, chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		blocks.solid_set(stockpileLocation, wood, false);
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		// Cannot find alternative stockpile project.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("delay by player")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .quantity=1u});
		items.setLocation(chunk1, chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, blocks.getIndex(5, 9, 1));
		actors.objective_addTaskToStart(dwarf1, std::move(objective));
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "stockpile");
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithProjectsCount() == 0);
	}
	SUBCASE("actor dies")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(chunk, wood);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile(queries);
		BlockIndex stockpileLocation = blocks.getIndex(5, 5, 1);
		BlockIndex chunkLocation = blocks.getIndex(1, 8, 1);
		stockpile.addBlock(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .quantity=1u});
		items.setLocation(chunk1, chunkLocation);
		area.m_hasStockPiles.at(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType, 100);
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		actors.die(dwarf1, CauseOfDeath::temperature);
		REQUIRE(area.m_hasStockPiles.at(faction).getStockPileFor(chunk1));
		REQUIRE(area.m_hasStockPiles.at(faction).getItemsWithProjectsCount() == 0);
	}
	SUBCASE("some but not all reserved item from stack area destroyed")
	{
		//TODO
	}
}
