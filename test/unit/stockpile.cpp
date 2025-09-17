#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/area/area.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/stockpile.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/project.h"
#include "../../engine/config.h"
#include "../../engine/haul.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/stockpile.h"
#include "../../engine/reference.h"
#include "../../engine/numericTypes/types.h"
#include <functional>
#include <memory>
TEST_CASE("stockpile")
{
	MaterialTypeId wood = MaterialType::byName("poplar wood");
	MaterialTypeId marble = MaterialType::byName("marble");
	MaterialTypeId gold = MaterialType::byName("gold");
	MaterialTypeId peridotite = MaterialType::byName("peridotite");
	MaterialTypeId sand = MaterialType::byName("sand");
	ItemTypeId chunk = ItemType::byName("chunk");
	ItemTypeId cart = ItemType::byName("cart");
	ItemTypeId boulder = ItemType::byName("boulder");
	ItemTypeId pile = ItemType::byName("pile");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	FactionId faction = simulation.m_hasFactions.createFaction("Tower of Power");
	area.m_hasStockPiles.registerFaction(faction);
	area.m_hasStocks.addFaction(faction);
	area.m_spaceDesignations.registerFaction(faction);
	ActorIndex dwarf1 = actors.create({
		.species=AnimalSpecies::byName("dwarf"),
		.location=Point3D::create(1, 1, 1),
		.faction=faction,
		.hasCloths=false,
		.hasSidearm=false,
	});
	ActorReference dwarf1Ref = area.getActors().m_referenceData.getReference(dwarf1);
	const StockPileObjectiveType& objectiveType = static_cast<const StockPileObjectiveType&>(ObjectiveType::getByName("stockpile"));
	SUBCASE("basic")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		CHECK(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		CHECK(area.m_hasStockPiles.getForFaction(faction).getHaulableItemForAt(dwarf1, chunkLocation).exists());
		StockPileObjective& objective = actors.objective_getCurrent<StockPileObjective>(dwarf1);
		CHECK(actors.move_hasPathRequest(dwarf1));
		AreaHasSpaceDesignationsForFaction& blockDesignations = area.m_spaceDesignations.getForFaction(faction);
		CHECK(blockDesignations.check(chunkLocation, SpaceDesignation::StockPileHaulFrom));
		CHECK(blockDesignations.check(stockpileLocation, SpaceDesignation::StockPileHaulTo));
		// Find item to stockpile
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		CHECK(objective.hasItem());
		CHECK(objective.hasDestination());
		// Verify path to stockpile, reserve item and create project.
		simulation.doStep();
		CHECK(objective.hasDestination());
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		CHECK(actors.project_exists(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		CHECK(project.hasWorker(dwarf1));
		CHECK(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		// Path to chunk.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		std::function<bool()> predicate = [&](){ return items.getLocation(chunk1) == stockpileLocation; };
		simulation.fastForwardUntillPredicate(predicate);
		CollisionVolume volume = Shape::getCollisionVolumeAtLocation(ItemType::getShape(chunk));
		CHECK(volume != 0);
		CHECK(space.shape_getStaticVolume(stockpileLocation) == volume);
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!objectiveType.canBeAssigned(area, dwarf1));
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("one worker hauls two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation1 = Point3D::create(5, 5, 1);
		Point3D stockpileLocation2 = Point3D::create(5, 6, 1);
		Point3D chunkLocation1 = Point3D::create(5, 0, 1);
		Point3D chunkLocation2 = Point3D::create(5, 9, 1);
		stockpile.addPoint(stockpileLocation1);
		stockpile.addPoint(stockpileLocation2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation1, .quantity=Quantity::create(1u)});
		ItemIndex chunk2 = items.create({.itemType=chunk, .materialType=marble, .location=chunkLocation2, .quantity=Quantity::create(1u)});
		CHECK(area.m_hasStockPiles.getForFaction(faction).getStockPileFor(chunk1) == &stockpile);
		CHECK(area.m_hasStockPiles.getForFaction(faction).getStockPileFor(chunk2) == &stockpile);
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk2);
		CHECK(area.m_hasStockPiles.getForFaction(faction).getItemsWithDestinations().size() == 2);
		CHECK(area.m_hasStockPiles.getForFaction(faction).getItemsWithDestinationsByStockPile().size() == 1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		std::function<bool()> predicate = [&](){ return items.getLocation(chunk1) == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		CHECK(objectiveType.canBeAssigned(area, dwarf1));
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		CHECK(area.m_hasStockPiles.getForFaction(faction).getItemsWithDestinations().size() == 1);
		CHECK(area.m_hasStockPiles.getForFaction(faction).getItemsWithDestinationsByStockPile().size() == 1);
		CHECK(!items.stockpile_canBeStockPiled(chunk1, faction));
		CHECK(items.stockpile_canBeStockPiled(chunk2, faction));
		// Find the second item and stockpile slot.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		StockPileObjective& objective = actors.objective_getCurrent<StockPileObjective>(dwarf1);
		CHECK(objective.getDestination() == stockpileLocation2);
		CHECK(objective.getItem() == area.getItems().m_referenceData.getReference(chunk2));
		// Verify spot, create project, reserve and activate.
		simulation.doStep();
		CHECK(actors.project_get(dwarf1)->getLocation() == stockpileLocation2);
		predicate = [&](){ return items.getLocation(chunk2) == stockpileLocation2 && !actors.project_exists(dwarf1); };
		simulation.fastForwardUntillPredicate(predicate);
		CHECK(!objectiveType.canBeAssigned(area, dwarf1));
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("Two workers haul two items")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation1 = Point3D::create(3, 4, 1);
		Point3D stockpileLocation2 = Point3D::create(3, 2, 1);
		Point3D chunkLocation1 = Point3D::create(1, 8, 1);
		Point3D chunkLocation2 = Point3D::create(9, 9, 1);
		stockpile.addPoint(stockpileLocation1);
		stockpile.addPoint(stockpileLocation2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=marble, .location=chunkLocation1, .quantity=Quantity::create(1u)});
		ItemIndex chunk2 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation2, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk2);
		ActorIndex dwarf2 = actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=Point3D::create(1, 2, 1),
			.faction=faction,
		});
		ActorReference dwarf2Ref = area.getActors().m_referenceData.getReference(dwarf2);
		CHECK(objectiveType.canBeAssigned(area, dwarf2));
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		actors.objective_setPriority(dwarf2, objectiveType.getId(), Priority::create(100));
		// Both dwarves find chunk 1
		simulation.doStep();
		// Destination is confirmed for both. Project is created, reserves and activates.
		simulation.doStep();
		// The worker which resered the project becomes dwarf1 (if not already) and the other dwarf2 and the other dwarf2.
		if(!actors.project_exists(dwarf1))
		{
			std::swap(dwarf1, dwarf2);
			std::swap(dwarf1Ref, dwarf2Ref);
		}
		StockPileProject& project = *static_cast<StockPileProject*>(actors.project_get(dwarf1));
		CHECK(project.hasTryToHaulThreadedTask());
		CHECK(project.getItem().getIndex(items.m_referenceData) == chunk1);
		//CHECK(project.getLocation() == stockpileLocation1);
		CHECK(project.hasWorker(dwarf1));
		CHECK(project.getMaxWorkers() == 1);
		CHECK(!project.hasWorker(dwarf2));
		CHECK(actors.objective_getCurrent<StockPileObjective>(dwarf1).m_project != nullptr);
		// Chunk1
		simulation.doStep();
		ActorReference project1WorkerRef;
		ActorReference project2WorkerRef;
		if(project.getWorkers().contains(dwarf1Ref))
		{
			project1WorkerRef = area.getActors().m_referenceData.getReference(dwarf1);
			project2WorkerRef = area.getActors().m_referenceData.getReference(dwarf2);
		}
		else
		{
			CHECK(project.getWorkers().contains(dwarf2Ref));
			project1WorkerRef = area.getActors().m_referenceData.getReference(dwarf2);
			project2WorkerRef = area.getActors().m_referenceData.getReference(dwarf1);
		}
		CHECK(!project.getWorkers().contains(project2WorkerRef));
		CHECK(project.reservationsComplete());
		CHECK(actors.move_hasPathRequest(dwarf2));
		// Project1 creates a haulsubproject, dwarf2 finds chunk2 and location2.
		simulation.doStep();
		CHECK(project.getWorkers().contains(project1WorkerRef));
		CHECK(project.getProjectWorkerFor(project1WorkerRef).haulSubproject);
		CHECK(actors.objective_getCurrent<StockPileObjective>(project1WorkerRef.getIndex(actors.m_referenceData)).m_project);
		CHECK(!project.getWorkers().contains(project2WorkerRef));
		CHECK(actors.objective_getCurrentName(project2WorkerRef.getIndex(actors.m_referenceData)) == "stockpile");
		CHECK(actors.project_exists(project2WorkerRef.getIndex(actors.m_referenceData)));
		StockPileProject& project2 = *static_cast<StockPileProject*>(actors.project_get(project2WorkerRef.getIndex(actors.m_referenceData)));
		CHECK(project2.getItem().getIndex(items.m_referenceData) == chunk2);
		CHECK(project2 != project);
		CHECK(project2.hasWorker(project2WorkerRef.getIndex(actors.m_referenceData)));
		CHECK(project2.getItem().getIndex(items.m_referenceData) == chunk2);
		CHECK(actors.move_getDestination(project1WorkerRef.getIndex(actors.m_referenceData)).exists());
		CHECK(items.isAdjacentToLocation(chunk1, actors.move_getDestination(project1WorkerRef.getIndex(actors.m_referenceData))));
		CHECK(actors.project_exists(project2WorkerRef.getIndex(actors.m_referenceData)));
		CHECK(actors.project_get(project2WorkerRef.getIndex(actors.m_referenceData)) != &project);
		CHECK(project2.getWorkers().contains(project2WorkerRef));
		CHECK(project2.reservationsComplete());
		// Project2 creates haul subproject.
		simulation.doStep();
		CHECK(project2.getWorkers().contains(project2WorkerRef));
		CHECK(project2.getProjectWorkerFor(project2WorkerRef).haulSubproject != nullptr);
		// Project2 paths.
		simulation.doStep();
		CHECK(actors.move_getDestination(project2WorkerRef.getIndex(actors.m_referenceData)).exists());
		std::function<bool()> predicate = [&](){
			const auto& location1 = items.getLocation(chunk1);
			const auto& location2 = items.getLocation(chunk2);
			return
				(location1 == stockpileLocation1 || location2 == stockpileLocation1) &&
				(location1 == stockpileLocation2 || location2 == stockpileLocation2);
		};
		simulation.fastForwardUntillPredicate(predicate);
		CHECK(actors.objective_getCurrentName(project1WorkerRef.getIndex(actors.m_referenceData)) != "stockpile");
		CHECK(actors.objective_getCurrentName(project2WorkerRef.getIndex(actors.m_referenceData)) != "stockpile");
	}
	SUBCASE("Team haul")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, gold));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation1 = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation1);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		CHECK(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		ActorIndex dwarf2 = actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=Point3D::create(1, 2, 1),
			.faction=faction,
		});
		ActorReference dwarf2Ref = area.getActors().m_referenceData.getReference(dwarf2);
		CHECK(objectiveType.canBeAssigned(area, dwarf1));
		CHECK(objectiveType.canBeAssigned(area, dwarf2));
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		actors.objective_setPriority(dwarf2, objectiveType.getId(), Priority::create(100));
		// Both actors find chunk1 and stockpileLocation1.
		simulation.doStep();
		// Both actors verify path as shortest.
		simulation.doStep();
		if(!actors.project_exists(dwarf1))
		{
			std::swap(dwarf1, dwarf2);
			std::swap(dwarf1Ref, dwarf2Ref);
		}
		Project& project = *actors.project_get(dwarf1);
		CHECK(project.hasWorker(dwarf1));
		CHECK(project.hasWorker(dwarf2));
		// Reserve target and destination.
		simulation.doStep();
		CHECK(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		CHECK(actors.project_exists(dwarf2));
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1Ref).haulSubproject;
		CHECK(&haulSubproject == project.getProjectWorkerFor(dwarf2Ref).haulSubproject);
		CHECK(haulSubproject.getHaulStrategy() == HaulStrategy::Team);
		std::function<bool()> predicate = [&](){ return items.getLocation(chunk1) == stockpileLocation1; };
		simulation.fastForwardUntillPredicate(predicate);
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
		CHECK(actors.objective_getCurrentName(dwarf2) != "stockpile");
	}
	SUBCASE("haul generic")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(pile, sand));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D cargoOrigin = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex cargo = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin, .quantity=Quantity::create(100u)});
		ItemReference cargoRef = items.getReference(cargo);
		Quantity quantity = actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(cargo), Config::minimumHaulSpeedInital);
		CHECK(quantity == 48);
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo);
		CHECK(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		CHECK(area.m_hasStockPiles.getForFaction(faction).getHaulableItemForAt(dwarf1, cargoOrigin).exists());
		AreaHasSpaceDesignationsForFaction& blockDesignations = area.m_spaceDesignations.getForFaction(faction);
		CHECK(blockDesignations.check(cargoOrigin, SpaceDesignation::StockPileHaulFrom));
		CHECK(blockDesignations.check(stockpileLocation, SpaceDesignation::StockPileHaulTo));
		// Find item to stockpile and potential destination.
		simulation.doStep();
		// Confirm destination, create project, and reserve.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		CHECK(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		// Path to cargo.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoOrigin);
		CHECK(actors.canPickUp_isCarryingItemGeneric(dwarf1, pile, sand, Quantity::create(48)));
		cargo = cargoRef.getIndex(items.m_referenceData);
		CHECK(space.item_contains(cargoOrigin, cargo));
		CHECK(items.getQuantity(cargo) == 52);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, stockpileLocation);
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		auto predicate = [&]{ return !space.item_empty(stockpileLocation); };
		simulation.fastForwardUntillPredicate(predicate);
		bool shouldBeTrue = space.stockpile_isAvalible(stockpileLocation, faction);
		CHECK(shouldBeTrue);
		CHECK(space.item_getCount(stockpileLocation, pile, sand) == 48);
		CollisionVolume volume = Shape::getCollisionVolumeAtLocation(ItemType::getShape(pile));
		CHECK(volume != 0);
		CHECK(space.shape_getStaticVolume(stockpileLocation) == volume * 48u);
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		CHECK(actors.move_hasPathRequest(dwarf1));
		CHECK(objectiveType.canBeAssigned(area, dwarf1));
		StockPileObjective& objective = actors.objective_getCurrent<StockPileObjective>(dwarf1);
		// Find potential item and destination.
		simulation.doStep();
		cargo = cargoRef.getIndex(items.m_referenceData);
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		CHECK(objective.hasItem());
		CHECK(objective.hasDestination());
		CHECK(objective.getItem().getIndex(items.m_referenceData) == cargo);
		CHECK(objective.getDestination() == stockpileLocation);
		CHECK(blockDesignations.check(cargoOrigin, SpaceDesignation::StockPileHaulFrom));
		CHECK(blockDesignations.check(stockpileLocation, SpaceDesignation::StockPileHaulTo));
		// Confirm destination.
		simulation.doStep();
		StockPileProject& project2 = static_cast<StockPileProject&>(*actors.project_get(dwarf1));
		CHECK(project2.getItem().getIndex(items.m_referenceData) == cargo);
		CHECK(objective.getDestination() == stockpileLocation);
		// select Haul strategy.
		simulation.doStep();
		CHECK(project2.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		CHECK(project2.getProjectWorkerFor(dwarf1Ref).haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		// Path to pickup.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoOrigin);
		cargo = cargoRef.getIndex(items.m_referenceData);
		cargoRef.clear();
		CHECK(actors.canPickUp_isCarryingItemGeneric(dwarf1, pile, sand, Quantity::create(48)));
		CHECK(items.getQuantity(cargo) == 4);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, stockpileLocation);
		auto predicate3 = [&]{ return space.item_getCount(stockpileLocation, pile, sand) == 96; };
		simulation.fastForwardUntillPredicate(predicate3);
		auto predicate4 = [&]{ return space.item_getCount(stockpileLocation, pile, sand) == 100; };
		simulation.fastForwardUntillPredicate(predicate4);
		CHECK(!actors.canPickUp_exists(dwarf1));
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("haul generic two workers")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(pile, sand));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation1 = Point3D::create(5, 5, 1);
		Point3D stockpileLocation2 = Point3D::create(5, 6, 1);
		Point3D cargoOrigin = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation1);
		stockpile.addPoint(stockpileLocation2);
		ItemIndex cargo = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin, .quantity=Quantity::create(200u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		ActorIndex dwarf2 = actors.create({
			.species=AnimalSpecies::byName("dwarf"),
			.location=Point3D::create(1, 2, 1),
			.faction=faction,
		});
		actors.objective_setPriority(dwarf2, objectiveType.getId(), Priority::create(100));
		// Both workers find hauling task.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		CHECK(actors.objective_getCurrentName(dwarf2) == "stockpile");
		// Confirm path
		simulation.doStep();
		// Dwarf1 creates a project.
		if(!actors.project_exists(dwarf1))
			std::swap(dwarf1, dwarf2);
		CHECK(actors.project_exists(dwarf1));
		CHECK(!actors.project_exists(dwarf2));
		// Dwarf2 finds the second stockpile location. Dwarf1 selects a haul strategy.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf2) == "stockpile");
		StockPileObjective& objective = actors.objective_getCurrent<StockPileObjective>(dwarf2);
		CHECK(objective.hasDestination());
		CHECK(objective.hasItem());
		auto& projectWorker = static_cast<StockPileProject&>(*actors.project_get(dwarf1)).getProjectWorkerFor(area.getActors().m_referenceData.getReference(dwarf1));
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// The second worker verifys drop off, creates project, makes reservations; the first paths to cargoOrigin.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf2) == "stockpile");
		CHECK(objective.getDestination() == stockpileLocation1);
		CHECK(actors.project_exists(dwarf2));
		StockPileProject& project = static_cast<StockPileProject&>(*actors.project_get(dwarf2));
		CHECK(project.getItem().getIndex(items.m_referenceData) == cargo);
		CHECK(cargoOrigin.isAdjacentTo(actors.move_getDestination(dwarf1)));
		// The second worker selects a haul strategy.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf2));
		CHECK(project.getProjectWorkerFor(area.getActors().m_referenceData.getReference(dwarf2)).haulSubproject != nullptr);
		CHECK(project.getProjectWorkerFor(area.getActors().m_referenceData.getReference(dwarf2)).haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// the second worker paths to cargoOrigin.
		simulation.doStep();
		CHECK(cargoOrigin.isAdjacentTo(actors.move_getDestination(dwarf2)));
		CHECK(cargoOrigin.isAdjacentTo(actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf2, cargoOrigin);
		CHECK(actors.canPickUp_exists(dwarf2));
		auto predicate1 = [&]{ return space.item_getCount(stockpileLocation1, pile, sand) != 0; };
		CHECK(actors.objective_getCurrentName(dwarf2) == "stockpile");
		simulation.fastForwardUntillPredicate(predicate1);
		Quantity firstDeliveryQuantity = space.item_getCount(stockpileLocation1, pile, sand);
		CHECK(actors.objective_getCurrentName(dwarf2) == "stockpile");
		auto predicate2 = [&]{ return space.item_getCount(stockpileLocation1, pile, sand) > firstDeliveryQuantity || space.item_getCount(stockpileLocation2, pile, sand) > firstDeliveryQuantity; };
		simulation.fastForwardUntillPredicate(predicate2);
		if(actors.getActionDescription(dwarf2) != "stockpile")
			CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
		else
			CHECK(actors.objective_getCurrentName(dwarf2) == "stockpile");
		auto predicate3 = [&]{ return space.item_getCount(stockpileLocation1, pile, sand) == 100 && space.item_getCount(stockpileLocation2, pile, sand) == 100; };
		simulation.fastForwardUntillPredicate(predicate3);
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!actors.project_exists(dwarf2));
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
		CHECK(actors.objective_getCurrentName(dwarf2) != "stockpile");
	}
	SUBCASE("haul generic two stockpile blocks and two piles")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(pile, sand));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation1 = Point3D::create(5, 5, 1);
		Point3D stockpileLocation2 = Point3D::create(6, 5, 1);
		Point3D cargoOrigin1 = Point3D::create(1, 8, 1);
		Point3D cargoOrigin2 = Point3D::create(2, 8, 1);
		stockpile.addPoint(stockpileLocation1);
		stockpile.addPoint(stockpileLocation2);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin1, .quantity=Quantity::create(100u)});
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=cargoOrigin2, .quantity=Quantity::create(100u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo1);
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo2);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, cargoOrigin1);
		CHECK(space.item_getCount(cargoOrigin1, pile, sand) < 100);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, stockpileLocation1);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, cargoOrigin1);
		CHECK(actors.project_exists(dwarf1));
		auto predicate = [&]{ return space.item_getCount(stockpileLocation1, pile, sand) == 100 && space.item_getCount(stockpileLocation2, pile, sand) == 100; };
		simulation.fastForwardUntillPredicate(predicate);
	}
	SUBCASE("haul generic stockpile is adjacent to cargo")
	{
		Point3D pileLocation = Point3D::create(5, 5, 1);
		Point3D stockpileLocation = Point3D::create(6, 5, 1);
		ItemIndex cargo = items.create({.itemType=pile, .materialType=sand, .location=pileLocation, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo);
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(pile, sand));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		stockpile.addPoint(stockpileLocation);
		CHECK(ObjectiveType::getByName("stockpile").canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// One step to find item and potential destination.
		simulation.doStep();
		CHECK(actors.move_hasPathRequest(dwarf1));
		// One step to verify, create, and reserve.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.project_get(dwarf1)->reservationsComplete());
		// Since the only required item is already adjacent to the project site deleveries are also complete.
		CHECK(actors.project_get(dwarf1)->deliveriesComplete());
		// Path to location.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, stockpileLocation);
		CHECK(actors.project_get(dwarf1)->finishEventExists());
		auto condition = [&]{ return space.item_getCount(stockpileLocation, pile, sand) == 15; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("haul generic stockpile is adjacent to two cargo piles")
	{
		Point3D pileLocation1 = Point3D::create(5, 5, 1);
		Point3D stockpileLocation = Point3D::create(6, 5, 1);
		Point3D pileLocation2 = Point3D::create(7, 5, 1);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation1, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo1);
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation2, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo2);
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(pile, sand));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		stockpile.addPoint(stockpileLocation);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		auto condition = [&]{ return space.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("haul generic consolidate adjacent")
	{
		Point3D pileLocation1 = Point3D::create(5, 5, 1);
		Point3D stockpileLocation = Point3D::create(6, 5, 1);
		Point3D pileLocation2 = Point3D::create(7, 5, 1);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation1, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo1);
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation2, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo2);
		ItemIndex cargo3 = items.create({.itemType=pile, .materialType=sand, .location=stockpileLocation, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo3);
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(pile, sand));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		stockpile.addPoint(stockpileLocation);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		auto condition = [&]{ return space.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
		CHECK(actors.getActionDescription(dwarf1) == "stockpile");
		CHECK(!actors.project_exists(dwarf1));
		CHECK(actors.move_hasPathRequest(dwarf1));
		// Step to find item and potential destination project.
		simulation.doStep();
		// Step to verify, create and reserve.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.project_get(dwarf1)->reservationsComplete());
		CHECK(actors.project_get(dwarf1)->deliveriesComplete());
		CHECK(actors.project_get(dwarf1)->finishEventExists());
		auto condition2 = [&]{ return space.item_getCount(stockpileLocation, pile, sand) == 45; };
		simulation.fastForwardUntillPredicate(condition2);
	}
	SUBCASE("tile which worker stands on for work phase of project contains item of the same generic type")
	{
		areaBuilderUtil::setSolidWall(area, Point3D::create(8, 1, 1), Point3D::create(9, 1, 1), marble);
		Point3D stockpileLocation = Point3D::create(9, 0, 1);
		Point3D pileLocation1 = Point3D::create(8, 0, 1);
		Point3D pileLocation2 = Point3D::create(7, 0, 1);
		ItemIndex cargo1 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation1, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo1);
		ItemIndex cargo2 = items.create({.itemType=pile, .materialType=sand, .location=pileLocation2, .quantity=Quantity::create(15)});
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo2);
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(pile, sand));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		stockpile.addPoint(stockpileLocation);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		auto condition = [&]{ return space.item_getCount(stockpileLocation, pile, sand) == 30; };
		simulation.fastForwardUntillPredicate(condition);
	}
	SUBCASE("path to item is blocked")
	{
		areaBuilderUtil::setSolidWall(area, Point3D::create(0, 3, 1), Point3D::create(8, 3, 1), wood);
		Point3D gateway = Point3D::create(9, 3, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find item to stockpile.
		simulation.doStep();
		// Confirm destination, create project, and reserve.
		simulation.doStep();
		Project& project = *actors.project_get(dwarf1);
		// Set haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject);
		// Path to chunk.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		CHECK(items.isAdjacentToLocation(chunk1, actors.move_getDestination(dwarf1)));
		auto path = actors.move_getPath(dwarf1);
		CHECK(path.contains(gateway));
		space.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
		CHECK(!items.reservable_isFullyReserved(chunk1, faction));
	}
	SUBCASE("path to stockpile is blocked")
	{
		areaBuilderUtil::setSolidWall(area, Point3D::create(0, 5, 1), Point3D::create(8, 5, 1), wood);
		Point3D gateway = Point3D::create(9, 5, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 8, 1);
		Point3D chunkLocation = Point3D::create(8, 1, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find item to stockpil.
		simulation.doStep();
		// Confirm destination, make project, reserve required.
		simulation.doStep();
		Project& project = *actors.project_get(dwarf1);
		// Set haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject);
		// Path to chunk.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		// Path to stockpile.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == "stockpile");
		CHECK(stockpileLocation.isAdjacentTo(actors.move_getDestination(dwarf1)));
		space.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		// Cannot detour or find alternative block.
		simulation.doStep();
		CHECK(!actors.canPickUp_isCarryingItem(dwarf1, chunk1));
		CHECK(!items.reservable_isFullyReserved(chunk1, faction));
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("path to haul tool is blocked")
	{
		areaBuilderUtil::setSolidWall(area, Point3D::create(0, 6, 1), Point3D::create(8, 6, 1), wood);
		Point3D gateway = Point3D::create(9, 6, 1);
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, gold));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 5, 1);
		Point3D cartLocation = Point3D::create(8, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=wood, .location=cartLocation, .quality=Quality::create(30), .percentWear=Percent::create(0)});
		CHECK(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		CHECK(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find candidates.
		simulation.doStep();
		// Reserve target and destination.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		CHECK(project.hasWorker(dwarf1));
		CHECK(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1Ref).haulSubproject;
		CHECK(haulSubproject.getHaulStrategy() == HaulStrategy::Cart);
		// Path to cart.
		simulation.doStep();
		CHECK(items.isAdjacentToLocation(cart1, actors.move_getDestination(dwarf1)));
		space.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour, cancel subproject.
		simulation.doStep();
		//TODO: Project should either reset or cancel subproject ranther then canceling the whole thing.
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("haul tool destroyed")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(boulder, peridotite));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 5, 1);
		Point3D cartLocation = Point3D::create(8, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex cargo = items.create({.itemType=boulder, .materialType=peridotite, .location=chunkLocation, .quantity=Quantity::create(1)});
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=wood, .location=cartLocation, .quality=Quality::create(30), .percentWear=Percent::create(0)});
		CHECK(!actors.canPickUp_item(dwarf1, cargo));
		area.m_hasStockPiles.getForFaction(faction).addItem(cargo);
		CHECK(objectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find candidates.
		simulation.doStep();
		// Confirm destination and reserve.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		CHECK(project.hasWorker(dwarf1));
		CHECK(project.reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		HaulSubproject& haulSubproject = *project.getProjectWorkerFor(dwarf1Ref).haulSubproject;
		CHECK(haulSubproject.getHaulStrategy() == HaulStrategy::Cart);
		items.destroy(cart1);
		CHECK(actors.project_get(dwarf1)->getWorkers()[area.getActors().m_referenceData.getReference(dwarf1)].haulSubproject == nullptr);
		CHECK(project.getHaulRetries() == 0);
		// Fast forward until haul project retry event spawns the haul retry threaded task.
		simulation.fastForward(Config::stepsFrequencyToLookForHaulSubprojects);
		CHECK(project.hasTryToHaulThreadedTask());
		CHECK(project.getHaulRetries() == 1);
		while(project.getHaulRetries() != Config::projectTryToMakeSubprojectRetriesBeforeProjectDelay - 1u)
		{
			simulation.fastForward(Config::stepsFrequencyToLookForHaulSubprojects);
			simulation.doStep();
		}
		simulation.fastForward(Config::stepsFrequencyToLookForHaulSubprojects);
		simulation.doStep();
		// Project has exhausted it's retry attempts.
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!items.reservable_isFullyReserved(cargo, faction));
		CHECK(!items.stockpile_canBeStockPiled(cargo, faction));
		// Dwarf1 tries to find another stockpile job but cannot.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("item destroyed")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		items.destroy(chunk1);
		CHECK(!actors.project_exists(dwarf1));
		// Cannot find alternative stockpile project.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("stockpile undesignated by player")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find item to stockpile.
		simulation.doStep();
		// Confirm destination, create project, reserve.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.project_get(dwarf1)->reservationsComplete());
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		stockpile.destroy();
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!objectiveType.canBeAssigned(area, dwarf1));
		CHECK(area.m_hasStockPiles.getForFaction(faction).getStockPileFor(chunk1) == nullptr);
		// Cannot find alternative stockpile project.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("destination set solid")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		space.solid_set(stockpileLocation, wood, false);
		CHECK(!actors.project_exists(dwarf1));
		// Cannot find alternative stockpile project.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
	}
	SUBCASE("delay by player")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(Point3D::create(5, 9, 1));
		actors.objective_addTaskToStart(dwarf1, std::move(objective));
		CHECK(!actors.project_exists(dwarf1));
		CHECK(actors.objective_getCurrentName(dwarf1) != "stockpile");
		CHECK(area.m_hasStockPiles.getForFaction(faction).getItemsWithProjectsCount() == 0);
	}
	SUBCASE("actor dies")
	{
		std::vector<ItemQuery> queries;
		queries.emplace_back(ItemQuery::create(chunk, wood));
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile(queries);
		Point3D stockpileLocation = Point3D::create(5, 5, 1);
		Point3D chunkLocation = Point3D::create(1, 8, 1);
		stockpile.addPoint(stockpileLocation);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=wood, .location=chunkLocation, .quantity=Quantity::create(1u)});
		area.m_hasStockPiles.getForFaction(faction).addItem(chunk1);
		actors.objective_setPriority(dwarf1, objectiveType.getId(), Priority::create(100));
		// Find item to stockpile, make project.
		simulation.doStep();
		// Reserve required.
		simulation.doStep();
		// Set haul strategy.
		simulation.doStep();
		// Path to chunk.
		simulation.doStep();
		actors.die(dwarf1, CauseOfDeath::temperature);
		CHECK(area.m_hasStockPiles.getForFaction(faction).getStockPileFor(chunk1));
		CHECK(area.m_hasStockPiles.getForFaction(faction).getItemsWithProjectsCount() == 0);
	}
	SUBCASE("some but not all reserved item from stack area destroyed")
	{
		//TODO
	}
}