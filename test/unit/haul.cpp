#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/project.h"
#include "../../engine/haul.h"
#include "../../engine/itemType.h"
#include "../../engine/targetedHaul.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/portables.hpp"
#include "reference.h"
#include "types.h"
TEST_CASE("haul")
{
	static MaterialTypeId dirt = MaterialType::byName(L"dirt");
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static MaterialTypeId gold = MaterialType::byName(L"gold");
	static MaterialTypeId lead = MaterialType::byName(L"lead");
	[[maybe_unused]] static MaterialTypeId iron = MaterialType::byName(L"iron");
	static MaterialTypeId poplarWood = MaterialType::byName(L"poplar wood");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	static AnimalSpeciesId donkey = AnimalSpecies::byName(L"jackstock donkey");
	static ItemTypeId chunk = ItemType::byName(L"chunk");
	static ItemTypeId boulder = ItemType::byName(L"boulder");
	static ItemTypeId cart = ItemType::byName(L"cart");
	static ItemTypeId panniers = ItemType::byName(L"panniers");
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
	ActorReference dwarf1Ref = actors.m_referenceData.getReference(dwarf1);
	CHECK(!actors.canPickUp_exists(dwarf1));
	SUBCASE("canPickup")
	{
		BlockIndex chunkLocation = blocks.getIndex_i(1, 2, 2);
		ItemIndex boulder1 = items.create({.itemType=boulder, .materialType=marble, .location=chunkLocation, .quantity=Quantity::create(1)});
		ItemIndex boulder2 = items.create({.itemType=boulder, .materialType=lead, .location=blocks.getIndex_i(6,2,2), .quantity=Quantity::create(1)});
		CHECK(actors.canPickUp_item(dwarf1, boulder1));
		CHECK(!actors.canPickUp_item(dwarf1, boulder2));
		actors.canPickUp_pickUpItem(dwarf1, boulder1);
		CHECK(items.getLocation(boulder1).empty());
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		actors.move_setDestinationAdjacentToLocation(dwarf1, destination);
		simulation.doStep();
		CHECK(!actors.move_getPath(dwarf1).empty());
		while(!actors.isAdjacentToLocation(dwarf1, destination))
			simulation.doStep();
		CHECK(actors.canPickUp_tryToPutDownItem(dwarf1, destination).exists());
		CHECK(blocks.item_getCount(destination, boulder, marble) == 1);
		CHECK(items.getLocation(boulder1).exists());
		CHECK(items.getLocation(boulder1) == destination);
	}
	SUBCASE("has haul tools")
	{
		BlockIndex cartLocation = blocks.getIndex_i(1, 2, 2);
		items.create({
			.itemType=cart,
			.materialType=poplarWood,
			.location=cartLocation,
			.quality=Quality::create(3u),
			.percentWear=Percent::create(0),
			.facing=Facing4::North,
		});
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .quantity=Quantity::create(1u)});
		CHECK(area.m_hasHaulTools.hasToolToHaulItem(area, faction, chunk1));
	}
	SUBCASE("individual haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=marble, .location=chunkLocation, .quantity=Quantity::create(1u)});
		ActorOrItemIndex cargo = ActorOrItemIndex::createForItem(chunk1);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1}), cargo, destination);
		CHECK(actors.objective_getCurrentName(dwarf1) == L"haul");
		CHECK(project.hasTryToAddWorkersThreadedTask());
		// One step to activate the project and make reservations.
		simulation.doStep();
		// One step to run the create subproject threaded task and set the strategy.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// Another step to find the path.
		simulation.doStep();
		CHECK(cargo.reservable_hasAny(area));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		CHECK(actors.canPickUp_exists(dwarf1));
		CHECK(actors.canPickUp_getPolymorphic(dwarf1) == cargo);
		CHECK(!cargo.reservable_hasAny(area));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(blocks.item_getCount(destination, chunk, marble) == 1);
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
		CHECK(!cargo.reservable_hasAny(area));
	}
	SUBCASE("hand cart haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		CHECK(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		BlockIndex cartLocation = blocks.getIndex_i(7, 7, 2);
		ItemIndex cart = items.create({.itemType=ItemType::byName(L"cart"), .materialType=MaterialType::byName(L"poplar wood"), .location=cartLocation, .quality=Quality::create(50u), .percentWear=Percent::create(0)});
		ActorOrItemIndex polymorphicChunk1 = ActorOrItemIndex::createForItem(chunk1);
		CHECK(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(area, dwarf1, cart, polymorphicChunk1, Config::minimumHaulSpeedInital) > 0);
		ActorOrItemIndex cargo = ActorOrItemIndex::createForItem(chunk1);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1}), cargo, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		CHECK(project.reservationsComplete());
		CHECK(project.getWorkers().contains(dwarf1Ref));
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(area, dwarf1, cart, polymorphicChunk1, project.getMinimumHaulSpeed()) > 0);
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Cart);
		// Another step to find the path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cartLocation);
		CHECK(actors.isLeadingItem(dwarf1, cart));
		// Another step to path from cart to chunk.
		simulation.doStep();
		CHECK(cargo.reservable_hasAny(area));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		CHECK(items.cargo_getItems(cart).size() == 1);
		CHECK(items.cargo_containsPolymorphic(cart, cargo));
		CHECK(!cargo.reservable_hasAny(area));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(blocks.item_getCount(destination, chunk, gold) == 1);
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(!items.cargo_exists(cart));
		CHECK(!items.reservable_isFullyReserved(cart, faction));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
	}
	SUBCASE("team haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(8, 8, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		ActorOrItemIndex cargo = ActorOrItemIndex::createForItem(chunk1);
		CHECK(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1, 2, 2),
			.faction=faction,
		});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1, dwarf2}), cargo, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		// Another step to find the paths.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Team);
		CHECK(actors.objective_getCurrentName(dwarf1) == L"haul");
		CHECK(actors.objective_getCurrentName(dwarf2) == L"haul");
		// Get into starting positions.
		simulation.fastForwardUntillActorHasNoDestination(area, dwarf2);
		if(!cargo.isAdjacentToActor(area, dwarf1))
			simulation.fastForwardUntillActorIsAdjacentToPolymorphic(area, dwarf1, cargo);
		if(actors.move_getDestination(dwarf1).exists())
			simulation.fastForwardUntillActorHasNoDestination(area, dwarf1);
		ActorIndex leader = dwarf1;
		ActorIndex follower = dwarf2;
		if(actors.isFollowing(dwarf2))
			CHECK(actors.isLeading(dwarf1));
		else
		{
			leader = dwarf2;
			follower = dwarf1;
			CHECK(actors.isFollowing(dwarf1));
			CHECK(actors.isLeading(dwarf2));
		}
		CHECK(cargo.isFollowing(area));
		CHECK(cargo.isLeading(area));
		// Another step to path.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, leader, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(blocks.item_getCount(destination, chunk, gold) == 1);
		CHECK(!actors.isFollowing(follower));
		CHECK(!actors.isLeadingActor(leader, follower));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
		CHECK(actors.objective_getCurrentName(dwarf2) != L"haul");
	}
	SUBCASE("panniers haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(8, 8, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		ActorOrItemIndex cargo = ActorOrItemIndex::createForItem(chunk1);
		ActorOrItemReference cargoRef = cargo.toReference(area);
		CHECK(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		BlockIndex donkeyLocation = blocks.getIndex_i(1, 2, 2);
		ActorIndex donkey1 = actors.create({
			.species=donkey,
			.location=donkeyLocation,
		});
		area.m_hasHaulTools.registerYokeableActor(area, donkey1);
		BlockIndex panniersLocation = blocks.getIndex_i(5, 1, 2);
		ItemIndex panniers1 = items.create({.itemType=panniers, .materialType=poplarWood, .location=panniersLocation, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1}), cargo, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		CHECK(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(area, dwarf1, donkey1, panniers1, cargo, project.getMinimumHaulSpeed()) > 0);
		auto haulParams = HaulSubproject::tryToSetHaulStrategy(project, cargoRef, dwarf1);
		CHECK(haulParams.strategy == HaulStrategy::Panniers);
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Panniers);
		// Another step to find the paths.
		simulation.doStep();
		CHECK(actors.move_getPath(dwarf1).size() != 0);
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, panniersLocation);
		simulation.doStep();
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, panniers1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, donkeyLocation);
		// Find path.
		simulation.doStep();
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(actors.equipment_containsItem(donkey1, panniers1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		// Find path.
		simulation.doStep();
		CHECK(items.cargo_containsPolymorphic(panniers1, cargo));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(blocks.item_getCount(destination, chunk, gold) == 1);
		CHECK(!actors.reservable_isFullyReserved(donkey1, faction));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
	}
	SUBCASE("panniers area already equiped haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(8, 8, 2);
		BlockIndex chunkLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex chunk1 = items.create({.itemType=chunk, .materialType=gold, .location=chunkLocation, .quantity=Quantity::create(1u)});
		ActorOrItemIndex cargo = ActorOrItemIndex::createForItem(chunk1);
		ActorOrItemReference cargoRef = cargo.toReference(area);
		CHECK(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(chunk1), Config::minimumHaulSpeedInital) == 0);
		BlockIndex donkeyLocation = blocks.getIndex_i(1, 3, 2);
		ActorIndex donkey1 = actors.create({
			.species=donkey,
			.location=donkeyLocation,
		});
		area.m_hasHaulTools.registerYokeableActor(area, donkey1);
		ItemIndex panniers1 = items.create({.itemType=panniers, .materialType=poplarWood, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		actors.equipment_add(donkey1, panniers1);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1}), cargo, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		CHECK(HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(area, dwarf1, donkey1, panniers1, cargo, project.getMinimumHaulSpeed()) > 0);
		auto haulParams = HaulSubproject::tryToSetHaulStrategy(project, cargoRef, dwarf1);
		CHECK(haulParams.strategy == HaulStrategy::Panniers);
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Panniers);
		// Another step to find the paths.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, donkeyLocation);
		CHECK(actors.isLeading(dwarf1));
		CHECK(actors.isFollowing(donkey1));
		// Find path.
		simulation.doStep();
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(actors.equipment_containsItem(donkey1, panniers1));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, chunkLocation);
		// Find path.
		simulation.doStep();
		CHECK(items.cargo_containsPolymorphic(panniers1, cargo));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(blocks.item_getCount(destination, chunk, gold) == 1);
		CHECK(!actors.reservable_isFullyReserved(donkey1, faction));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
	}
	SUBCASE("animal cart haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		BlockIndex boulderLocation = blocks.getIndex_i(1, 5, 2);
		ItemIndex boulder1 = items.create({.itemType=boulder, .materialType=lead, .location=boulderLocation, .quantity=Quantity::create(1u)});
		ActorOrItemIndex cargo = ActorOrItemIndex::createForItem(boulder1);
		BlockIndex donkeyLocation = blocks.getIndex_i(4, 3, 2);
		ActorIndex donkey1 = actors.create({
			.species=donkey,
			.location=donkeyLocation,
		});
		area.m_hasHaulTools.registerYokeableActor(area, donkey1);
		BlockIndex cartLocation = blocks.getIndex_i(5, 1, 2);
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=poplarWood, .location=cartLocation, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1}), cargo, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::AnimalCart);
		// Another step to find the paths.
		simulation.doStep();
		CHECK(actors.isAdjacentToLocation(donkey1, actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToActor(area, dwarf1, donkey1);
		simulation.doStep();
		CHECK(actors.isLeadingActor(dwarf1, donkey1));
		CHECK(actors.move_getPath(dwarf1).size() != 0);
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cartLocation);
		CHECK(actors.isLeadingItem(donkey1, cart1));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, boulderLocation);
		CHECK(items.cargo_containsPolymorphic(cart1, cargo));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(blocks.item_getCount(destination, boulder, lead) == 1);
		CHECK(!actors.reservable_isFullyReserved(donkey1, faction));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
	}
	SUBCASE("team hand cart haul strategy")
	{
		BlockIndex destination = blocks.getIndex_i(9, 9, 2);
		BlockIndex cargoLocation = blocks.getIndex_i(1, 7, 2);
		ItemIndex cargo1 = items.create({.itemType=boulder, .materialType=iron, .location=cargoLocation});
		ActorOrItemIndex cargo = ActorOrItemIndex::createForItem(cargo1);
		BlockIndex origin2 = blocks.getIndex_i(4, 3, 2);
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=origin2,
			.faction=faction,
		});
		ActorReference dwarf2Ref = actors.m_referenceData.getReference(dwarf2);
		BlockIndex cartLocation = blocks.getIndex_i(7, 1, 2);
		ItemIndex cart1 = items.create({.itemType=cart, .materialType=poplarWood, .location=cartLocation, .quality=Quality::create(3u), .percentWear=Percent::create(0)});
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1, dwarf2}), cargo, destination);
		// One step to activate the project and make reservations.
		simulation.doStep();
		// Another step to select the haul strategy and create the subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::TeamCart);
		ProjectWorker& projectWorker2 = project.getProjectWorkerFor(dwarf2Ref);
		CHECK(projectWorker2.haulSubproject != nullptr);
		CHECK(projectWorker2.haulSubproject->getHaulStrategy() == HaulStrategy::TeamCart);
		// Another step to find the paths.
		simulation.doStep();
		// Get into starting positions.
		if(actors.move_getPath(dwarf1).empty() || actors.move_getPath(dwarf2).empty())
			// The two dwarves pathed to the same place, one needs to repath.
			simulation.doStep();
		BlockIndex destination1 = actors.move_getDestination(dwarf1);
		BlockIndex destination2 = actors.move_getDestination(dwarf2);
		CHECK(destination1.exists());
		CHECK(destination2.exists());
		CHECK(items.isAdjacentToLocation(cart1, destination1));
		CHECK(items.isAdjacentToLocation(cart1, destination2));
		CHECK(destination1 != destination2);
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, destination1);
		if(actors.getLocation(dwarf2) != destination2)
			simulation.fastForwardUntillActorIsAtDestination(area, dwarf2, destination2);
		CHECK(actors.isLeadingItem(dwarf1, cart1));
		CHECK(items.isLeadingActor(cart1, dwarf2));
		CHECK(actors.isFollowing(dwarf2));
		// Find path
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoLocation);
		CHECK(items.cargo_containsItem(cart1, cargo1));
		// Path a detour around dwarf2 again.
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(blocks.item_getCount(destination, boulder, iron) == 1);
		CHECK(!items.reservable_isFullyReserved(cart1, faction));
		CHECK(!actors.isLeadingItem(dwarf1, cart1));
		CHECK(!actors.isFollowing(dwarf2));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
	}
	SUBCASE("individual haul actor")
	{
		BlockIndex destination = blocks.getIndex_i(5, 5, 2);
		BlockIndex cargoOrigin = blocks.getIndex_i(1, 5, 2);
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.percentGrown=Percent::create(30),
			.location=cargoOrigin,
			.faction=faction,
			.hasCloths=false,
			.hasSidearm=false
		});
		CHECK(actors.canPickUp_actor(dwarf1, dwarf2));
		ActorOrItemIndex cargo = ActorOrItemIndex::createForActor(dwarf2);
		TargetedHaulProject& project = area.m_hasTargetedHauling.begin(SmallSet<ActorIndex>({dwarf1}), cargo, destination);
		CHECK(actors.move_getIndividualSpeedWithAddedMass(dwarf1, actors.getMass(dwarf2)) >= project.getMinimumHaulSpeed());
		CHECK(actors.canPickUp_speedIfCarryingQuantity(dwarf1, actors.getMass(dwarf2), Quantity::create(1)) >= project.getMinimumHaulSpeed());
		CHECK(actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, actors.getMass(dwarf2), project.getMinimumHaulSpeed()) > 0);
		CHECK(actors.objective_getCurrentName(dwarf1) == L"haul");
		CHECK(project.hasTryToAddWorkersThreadedTask());
		// One step to activate the project and make reservations.
		simulation.doStep();
		CHECK(cargo.reservable_exists(area, faction));
		CHECK(project.hasTryToHaulThreadedTask());
		// One step to run the create subproject threaded task and set the strategy.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// Another step to find the path.
		simulation.doStep();
		CHECK(cargo.reservable_exists(area, faction));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, cargoOrigin);
		CHECK(actors.canPickUp_exists(dwarf1));
		CHECK(actors.canPickUp_getPolymorphic(dwarf1) == cargo);
		CHECK(!cargo.reservable_hasAny(area));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, destination);
		simulation.fastForward(Config::addToStockPileDelaySteps);
		CHECK(actors.getLocation(dwarf2) == destination);
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"haul");
		CHECK(!cargo.reservable_hasAny(area));
	}
}
