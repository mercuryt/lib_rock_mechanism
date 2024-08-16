#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/project.h"
#include "../../engine/haul.h"
#include "../../engine/itemType.h"
#include "../../engine/targetedHaul.h"
#include "../../engine/animalSpecies.h"
#include "reference.h"
#include "types.h"
TEST_CASE("haul")
{
	static MaterialTypeId dirt = MaterialType::byName("dirt");
	static MaterialTypeId marble = MaterialType::byName("marble");
	static MaterialTypeId gold = MaterialType::byName("gold");
	static MaterialTypeId lead = MaterialType::byName("lead");
	[[maybe_unused]] static MaterialTypeId iron = MaterialType::byName("iron");
	static MaterialTypeId poplarWood = MaterialType::byName("poplar wood");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	static AnimalSpeciesId donkey = AnimalSpecies::byName("jackstock donkey");
	static ItemTypeId chunk = ItemType::byName("chunk");
	static ItemTypeId boulder = ItemType::byName("boulder");
	static ItemTypeId cart = ItemType::byName("cart");
	static ItemTypeId panniers = ItemType::byName("panniers");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	FactionId faction = simulation.createFaction(L"Tower Of Power");
	ActorIndex dwarf1 = actors.create({
		.species=dwarf, 
		.location=blocks.getIndex_i(1, 1, 2),
		.faction=faction,
		.hasCloths=false,
		.hasSidearm=false
	});
	ActorReference dwarf1Ref = dwarf1.toReference(area);
	REQUIRE(!actors.canPickUp_exists(dwarf1));
	SUBCASE("canPickup")
	{
		BlockIndex chunkLocation = blocks.getIndex_i(1, 2, 2);
		ItemIndex boulder1 = items.create({.itemType=boulder, .materialType=marble, .location=chunkLocation, .quantity=Quantity::create(1)});
		ItemIndex boulder2= items.create({.itemType=boulder, .materialType=lead, .location=blocks.getIndex_i(6,2,2), .quantity=Quantity::create(1)});
		REQUIRE(actors.canPickUp_item(dwarf1, boulder1));
		REQUIRE(!actors.canPickUp_item(dwarf1, boulder2));
		actors.canPickUp_pickUpItem(dwarf1, boulder1);
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		actors.move_setDestination(dwarf1, destination);
		simulation.doStep();
		REQUIRE(!actors.move_getPath(dwarf1).empty());
		while(actors.getLocation(dwarf1) != destination)
			simulation.doStep();
		auto item = actors.canPickUp_tryToPutDownItem(dwarf1, actors.getLocation(dwarf1));
		assert(item.exists());
		REQUIRE(blocks.item_getCount(destination, boulder, marble) == 1);
	}
	SUBCASE("has haul tools")
	{
		BlockIndex cartLocation = blocks.getIndex_i(1, 2, 2);
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=poplarWood, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .quantity=Quantity::create(1u)});
		items.setLocation(cart1, cartLocation);
		REQUIRE(area.m_hasHaulTools.hasToolToHaulItem(area, faction, chunk1));
	}
	SUBCASE("individual haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=marble, .location=chunkLocation, .quantity=Quantity::create(1u)});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(ActorIndices({dwarf1}), chunk1, destination);
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "haul");
		REQUIRE(simulation.m_threadedTaskEngine.count() == 1);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// One step to run the create subproject threaded task and set the strategy.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// Another step to find the path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		REQUIRE(actors.canPickUp_exists(dwarf1));
		REQUIRE(actors.canPickUp_getItem(dwarf1) == chunk1);
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(blocks.item_getCount(destination, chunk, marble) == 1);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "haul");
	}
	SUBCASE("hand cart haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		REQUIRE(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		BlockIndex cartLocation = blocks.getIndex_i(7, 7, 2);
		ItemIndex cart = items.create({.itemType=ItemType::byName("cart"), .materialType=MaterialType::byName("poplar wood"), .location=cartLocation, .quality=Quality::create(50u), .percentWear=Percent::create(0)});
		ActorOrItemIndex polymorphicChunk1 = ActorOrItemIndex::createForItem(chunk1);
		REQUIRE(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(area, dwarf1, cart, polymorphicChunk1, Config::minimumHaulSpeedInital) > 0);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(ActorIndices({dwarf1}), chunk1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		REQUIRE(project.getWorkers().contains(dwarf1Ref));
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		REQUIRE(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(area, dwarf1, cart, polymorphicChunk1, project.getMinimumHaulSpeed()) > 0);
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Cart);
		// Another step to find the path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cartLocation);
		REQUIRE(actors.isLeadingItem(dwarf1, cart));
		// Another step to path from cart to chunk.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		REQUIRE(items.cargo_getItems(cart).size() == 1);
		REQUIRE(*(items.cargo_getItems(cart).begin()) == chunk1);
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(blocks.item_getCount(destination, chunk, gold) == 1);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		REQUIRE(!items.cargo_exists(cart));
		REQUIRE(!items.reservable_isFullyReserved(cart, faction));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "haul");
	}
	SUBCASE("team haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(8, 8, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		REQUIRE(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1, 2, 2),
			.faction=faction,
		});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(ActorIndices({dwarf1, dwarf2}), chunk1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		// Another step to find the paths.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Team);
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "haul");
		REQUIRE(actors.objective_getCurrentName(dwarf2) == "haul");
		// Get into starting positions.
		simulation.fastForwardUntillActorHasNoDestination(area, dwarf2);
		if(!actors.isAdjacentToItem(dwarf1, chunk1))
			simulation.fastForwardUntillActorIsAdjacentToItem(area, dwarf1, chunk1);
		if(actors.move_getDestination(dwarf1).exists())
			simulation.fastForwardUntillActorHasNoDestination(area, dwarf1);
		REQUIRE(actors.isFollowing(dwarf2));
		REQUIRE(actors.isLeading(dwarf1));
		// Another step to path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(blocks.item_getCount(destination, chunk, gold) == 1);
		REQUIRE(!actors.isFollowing(dwarf2));
		REQUIRE(!actors.isLeadingActor(dwarf1, dwarf2));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "haul");
		REQUIRE(actors.objective_getCurrentName(dwarf2) != "haul");
	}
	SUBCASE("panniers haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(8, 8, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		items.setLocation(chunk1, chunkLocation);
		REQUIRE(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		BlockIndex donkeyLocation = blocks.getIndex_i(1, 2, 2);
		ActorIndex donkey1 = actors.create({
			.species=donkey,
			.location=donkeyLocation,
		});
		area.m_hasHaulTools.registerYokeableActor(area, donkey1);
		BlockIndex panniersLocation = blocks.getIndex_i(5, 1, 2);
		ItemIndex panniers1 = items.create({.itemType=panniers, .materialType=poplarWood, .location=panniersLocation, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(ActorIndices({dwarf1}), chunk1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		ActorOrItemIndex polymorphicChunk1 = ActorOrItemIndex::createForItem(chunk1);
		REQUIRE(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(area, dwarf1, donkey1, panniers1, polymorphicChunk1, project.getMinimumHaulSpeed()) > 0);
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Panniers);
		// Another step to find the paths.
		simulation.doStep();
		REQUIRE(actors.move_getPath(dwarf1).size() != 0);
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, panniersLocation);
		simulation.doStep();
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, panniers1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, donkeyLocation);
		// Find path.
		simulation.doStep();
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		REQUIRE(actors.equipment_containsItem(donkey1, panniers1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		// Find path.
		simulation.doStep();
		REQUIRE(items.cargo_containsItem(panniers1, chunk1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(blocks.item_getCount(destination, chunk, gold) == 1);
		REQUIRE(!actors.reservable_isFullyReserved(donkey1, faction));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "haul");
	}
	SUBCASE("animal cart haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		BlockIndex boulderLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex boulder1 = items.create({.itemType=boulder, .materialType=lead, .location=boulderLocation, .quantity=Quantity::create(1u)});
		items.setLocation(boulder1, boulderLocation);
		BlockIndex donkeyLocation = blocks.getIndex_i(4, 3, 2);
		ActorIndex donkey1 = actors.create({
			.species=donkey,
			.location=donkeyLocation,
		});
		area.m_hasHaulTools.registerYokeableActor(area, donkey1);
		BlockIndex cartLocation = blocks.getIndex_i(5, 1, 2);
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=poplarWood, .location=cartLocation, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(ActorIndices({dwarf1}), boulder1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::AnimalCart);
		// Another step to find the paths.
		simulation.doStep();
		REQUIRE(actors.isAdjacentToLocation(donkey1, actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToActor(area, dwarf1, donkey1);
		simulation.doStep();
		REQUIRE(actors.isLeadingActor(dwarf1, donkey1));
		REQUIRE(actors.move_getPath(dwarf1).size() != 0);
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cartLocation);
		REQUIRE(actors.isLeadingItem(donkey1, cart1));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, boulderLocation);
		REQUIRE(items.cargo_containsItem(cart1, boulder1));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(blocks.item_getCount(destination, boulder, lead) == 1);
		REQUIRE(!actors.reservable_isFullyReserved(donkey1, faction));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "haul");
	}
	SUBCASE("team hand cart haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(9, 9, 2);
		BlockIndex cargoLocation = blocks.getIndex_i(1, 7, 2);
		ItemIndex cargo1 = items.create({.itemType=boulder, .materialType=iron, .location=cargoLocation});
		BlockIndex origin2 = blocks.getIndex_i(4, 3, 2);
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=origin2,
			.faction=faction,
		});
		ActorReference dwarf2Ref = dwarf2.toReference(area);
		BlockIndex cartLocation = blocks.getIndex_i(7, 1, 2);
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=poplarWood, .location=cartLocation, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		items.setLocation(cart1, cartLocation);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(ActorIndices({dwarf1, dwarf2}), cargo1, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::TeamCart);
		ProjectWorker& projectWorker2 = project.getProjectWorkerFor(dwarf2Ref);
		REQUIRE(projectWorker2.haulSubproject != nullptr);
		REQUIRE(projectWorker2.haulSubproject->getHaulStrategy() == HaulStrategy::TeamCart);
		// Another step to find the paths.
		simulation.doStep();
		// Get into starting positions.
		if(actors.move_getPath(dwarf1).empty() || actors.move_getPath(dwarf2).empty())
			// The two dwarves pathed to the same place, one needs to repath.
			simulation.doStep();
		BlockIndex destination1 = actors.move_getDestination(dwarf1);
		BlockIndex destination2 = actors.move_getDestination(dwarf2);
		REQUIRE(destination1.exists());
		REQUIRE(destination2.exists());
		REQUIRE(items.isAdjacentToLocation(cart1, destination1));
		REQUIRE(items.isAdjacentToLocation(cart1, destination2));
		REQUIRE(destination1 != destination2);
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		if(actors.getLocation(dwarf2) != destination2)
			simulation.fastForwardUntillActorIsAtDestination(area, dwarf2, destination2);
		REQUIRE(actors.isLeadingItem(dwarf1, cart1));
		REQUIRE(items.isLeadingActor(cart1, dwarf2));
		REQUIRE(actors.isFollowing(dwarf2));
		// Find path
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoLocation);
		REQUIRE(items.cargo_containsItem(cart1, cargo1));
		// Path a detour around dwarf2 again.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(blocks.item_getCount(destination, boulder, iron) == 1);
		REQUIRE(!items.reservable_isFullyReserved(cart1, faction));
		REQUIRE(!actors.isLeadingItem(dwarf1, cart1));
		REQUIRE(!actors.isFollowing(dwarf2));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "haul");
	}
}
