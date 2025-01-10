#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/items/items.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/construct.h"
#include "../../engine/cuboid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/config.h"
#include "../../engine/materialType.h"
#include "../../engine/types.h"
#include "../../engine/itemType.h"
#include "../../engine/objectives/construct.h"
#include "../../engine/items/items.h"
#include "../../engine/actors/actors.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/plants.h"
#include "../../engine/animalSpecies.h"
#include "reference.h"
TEST_CASE("construct")
{
	static MaterialTypeId wood = MaterialType::byName(L"poplar wood");
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
	FactionId faction = simulation.createFaction(L"Tower Of Power");
	area.m_blockDesignations.registerFaction(faction);
	const ConstructObjectiveType& constructObjectiveType = static_cast<const ConstructObjectiveType&>(ObjectiveType::getByName(L"construct"));
	BlockIndex dwarf1Start = blocks.getIndex_i(1, 1, 2);
	ActorIndex dwarf1 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=dwarf1Start,
		.faction=faction,
	});
	ActorReference dwarf1Ref = actors.m_referenceData.getReference(dwarf1);
	area.m_hasConstructionDesignations.addFaction(faction);
	ItemIndex boards = items.create({
		.itemType=ItemType::byName(L"board"),
		.materialType=wood,
		.location=blocks.getIndex_i(8,7,2),
		.quantity=Quantity::create(50u),
	});
	ItemIndex pegs = items.create({
		.itemType=ItemType::byName(L"peg"),
		.materialType=wood,
		.location=blocks.getIndex_i(3, 8, 2),
		.quantity=Quantity::create(50u)
	});
	ItemIndex saw = items.create({
		.itemType=ItemType::byName(L"saw"),
		.materialType=MaterialType::byName(L"bronze"),
		.location=blocks.getIndex_i(5,7,2),
		.quality=Quality::create(25u),
		.percentWear=Percent::create(0)
	});
	ItemIndex mallet = items.create({
		.itemType=ItemType::byName(L"mallet"),
		.materialType=wood,
		.location=blocks.getIndex_i(9, 5, 2),
		.quality=Quality::create(25u),
		.percentWear=Percent::create(0),
	});
	SUBCASE("make wall")
	{
		BlockIndex wallLocation = blocks.getIndex_i(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		REQUIRE(blocks.designation_has(wallLocation, faction, BlockDesignation::Construct));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation));
		REQUIRE(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == L"construct");
		ConstructObjective& objective = actors.objective_getCurrent<ConstructObjective>(dwarf1);
		REQUIRE(objective.joinableProjectExistsAt(area, wallLocation, dwarf1));
		const auto& terrainFacade = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(dwarf1));
		REQUIRE(terrainFacade.accessable(dwarf1Start, actors.getFacing(dwarf1), wallLocation, dwarf1));
		// Search for accessable project, activete project and reserve all required.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) == &project);
		REQUIRE(project.reservationsComplete());
		REQUIRE(project.hasTryToHaulThreadedTask());
		// Four Item types are required but the mallet is already adjacent to the location.
		REQUIRE(project.getToPickup().size() == 3);
		// Select a haul strategy and create a subproject.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// Find a path to the first item.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		REQUIRE(actors.canPickUp_exists(dwarf1));
		// Path to the project location.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		REQUIRE(project.getToPickup().size() == 2);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		REQUIRE(project.hasTryToHaulThreadedTask());
		REQUIRE(projectWorker.haulSubproject == nullptr);
		// Create haulSubproject.
		simulation.doStep();
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(actors.move_hasPathRequest(dwarf1));
		// Path to the second item.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) == L"construct");
		REQUIRE(!actors.move_hasPathRequest(dwarf1));
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		REQUIRE(actors.canPickUp_exists(dwarf1));
		// Path to the project location.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		REQUIRE(project.getToPickup().size() == 1);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		// Create haulSubproject.
		simulation.doStep();
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(actors.move_hasPathRequest(dwarf1));
		// Path to the third item.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) == L"construct");
		REQUIRE(!actors.move_hasPathRequest(dwarf1));
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		REQUIRE(actors.canPickUp_exists(dwarf1));
		// Path to the project location.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		REQUIRE(project.getToPickup().empty());
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		REQUIRE(project.deliveriesComplete());
		// Deliveries complete, start construction.
		REQUIRE(project.finishEventExists());
		simulation.fastForwardUntillPredicate([&]()->bool { return blocks.solid_is(wallLocation); }, 180);
		REQUIRE(actors.objective_getCurrentName(dwarf1) != L"construct");
		REQUIRE(!area.m_hasConstructionDesignations.contains(faction, wallLocation));
		REQUIRE(!constructObjectiveType.canBeAssigned(area, dwarf1));
		REQUIRE(!blocks.designation_has(wallLocation, faction, BlockDesignation::Construct));
		REQUIRE(items.reservable_getUnreservedCount(boards, faction) == items.getQuantity(boards));
		REQUIRE(items.reservable_getUnreservedCount(pegs, faction) == items.getQuantity(pegs));
		REQUIRE(!items.reservable_isFullyReserved(saw, faction));
		REQUIRE(!items.reservable_isFullyReserved(mallet, faction));
		REQUIRE(!blocks.isReserved(wallLocation, faction));
	}
	SUBCASE("make two walls")
	{
		BlockIndex wallLocation1 = blocks.getIndex_i(8, 4, 2);
		BlockIndex wallLocation2 = blocks.getIndex_i(8, 5, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, nullptr, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		REQUIRE(actors.objective_getCurrentName(dwarf1) == L"construct");
		ConstructProject& project1 = area.m_hasConstructionDesignations.getProject(faction, wallLocation1);
		ConstructProject& project2 = area.m_hasConstructionDesignations.getProject(faction, wallLocation2);
		// Find project to join, activate and reserve.
		simulation.doStep();
		REQUIRE(project1.reservationsComplete());
		simulation.fastForwardUntillPredicate([&]()->bool { return project1.deliveriesComplete(); });
		simulation.fastForwardUntillPredicate([&]()->bool { return blocks.solid_is(wallLocation1); }, 180);
		simulation.doStep();
		REQUIRE(project2.reservationsComplete());
		simulation.fastForwardUntillPredicate([&]()->bool { return project2.deliveriesComplete(); });
		simulation.fastForwardUntillPredicate([&]()->bool { return blocks.solid_is(wallLocation2); }, 180);
		REQUIRE(actors.objective_getCurrentName(dwarf1) != L"construct");
	}
	SUBCASE("make wall with two workers")
	{
		ActorIndex dwarf2 = actors.create(ActorParamaters{
			.species=dwarf,
			.location=blocks.getIndex_i(1, 4, 2),
			.faction=faction
		});
		BlockIndex wallLocation = blocks.getIndex_i(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		actors.objective_setPriority(dwarf2, constructObjectiveType.getId(), Priority::create(100));;
		REQUIRE(actors.objective_getCurrentName(dwarf1) == L"construct");
		REQUIRE(actors.objective_getCurrentName(dwarf2) == L"construct");
		// Find project to join.
		std::function<bool()> predicate = [&]() { return blocks.solid_is(wallLocation); };
		simulation.fastForwardUntillPredicate(predicate, 90);
		REQUIRE(actors.objective_getCurrentName(dwarf1) != L"construct");
		REQUIRE(actors.objective_getCurrentName(dwarf2) != L"construct");
	}
	SUBCASE("make two walls with two workers")
	{
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1, 8, 2),
			.faction=faction,
		});
		ActorReference dwarf2Ref = actors.m_referenceData.getReference(dwarf2);
		BlockIndex wallLocation1 = blocks.getIndex_i(2, 4, 2);
		BlockIndex wallLocation2 = blocks.getIndex_i(3, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, nullptr, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		actors.objective_setPriority(dwarf2, constructObjectiveType.getId(), Priority::create(100));;
		REQUIRE(actors.objective_getCurrentName(dwarf1) == L"construct");
		REQUIRE(actors.objective_getCurrentName(dwarf2) == L"construct");
		ConstructProject* project1 = &area.m_hasConstructionDesignations.getProject(faction, wallLocation1);
		ConstructProject* project2 = &area.m_hasConstructionDesignations.getProject(faction, wallLocation2);
		REQUIRE(actors.objective_getCurrent<ConstructObjective>(dwarf2).getProjectWhichActorCanJoinAt(area, wallLocation2, dwarf2));
		REQUIRE(actors.move_hasPathRequest(dwarf2));
		// Find projects to join.
		// Activate a project with one dwarf and reserve all required, the other dwarf fails to validate their project due to tools being reserved.
		// The project which was reserved becomes project1 (if not already) and the other becomes project2.
		// The dwarf which reserved becosem dwarf1 and the other becomes dwarf2.
		simulation.doStep();
		if(project2->reservationsComplete())
		{
			std::swap(project1, project2);
			std::swap(dwarf1, dwarf2);
			std::swap(wallLocation1, wallLocation2);
			std::swap(dwarf1Ref, dwarf2Ref);
		}
		REQUIRE(actors.objective_getCurrent<ConstructObjective>(dwarf2).getProjectWhichActorCanJoinAt(area, wallLocation2, dwarf2));
		REQUIRE(actors.project_get(dwarf1) == project1);
		REQUIRE(actors.project_get(dwarf2) == project2);
		REQUIRE(project1->reservationsComplete());
		REQUIRE(!project2->reservationsComplete());
		REQUIRE(project1->getWorkers().size() == 1);
		REQUIRE(project2->getWorkers().empty());
		REQUIRE(!project1->hasCandidate(dwarf1));
		REQUIRE(project2->hasCandidate(dwarf2));
		// Select a haul strategy and create a subproject for dwarf1, dwarf2 tries again to activate project2, this time failing to find required unreserved items and activating prohibition on the project at the objective instance.
		simulation.doStep();
		REQUIRE(!actors.objective_getCurrent<ConstructObjective>(dwarf2).getProjectWhichActorCanJoinAt(area, wallLocation2, dwarf2));
		ProjectWorker& projectWorker1 = project1->getProjectWorkerFor(dwarf1Ref);
		REQUIRE(projectWorker1.haulSubproject != nullptr);
		REQUIRE(project1->getWorkers().size() == 1);
		REQUIRE(!project2->hasCandidate(dwarf2));
		REQUIRE(!actors.project_exists(dwarf2));
		// Find a path for dwarf1, dwarf2 seeks project to join, and finds project1.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1) .exists());
		REQUIRE(actors.project_get(dwarf1) == project1);
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation1));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation2));
		REQUIRE(project1->getWorkers().contains(dwarf2Ref));
		// Build first wall segment.
		simulation.fastForwardUntillPredicate([&]()->bool { return blocks.solid_is(wallLocation1); }, 90);
		REQUIRE(!area.m_hasConstructionDesignations.contains(faction, wallLocation1));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation2));
		REQUIRE(!project2->isOnDelay());
		REQUIRE(blocks.designation_has(project2->getLocation(), faction, BlockDesignation::Construct));
		// Both dwarves seek a project to join and find project2. This does not require a step if they are already adjacent but does if they are not.
		REQUIRE(actors.objective_getCurrentName(dwarf1) == L"construct");
		REQUIRE(actors.objective_getCurrentName(dwarf2) == L"construct");
		simulation.doStep();
		// Project2 completes reservations and both dwarfs graduate from candidate to worker.
		simulation.doStep();
		REQUIRE(project2->getWorkers().contains(dwarf1Ref));
		REQUIRE(project2->getWorkers().contains(dwarf2Ref));
		REQUIRE(!project2->hasCandidate(dwarf1));
		REQUIRE(!project2->hasCandidate(dwarf2));
		// Try to set haul strategy, both dwarves try to generate a haul subproject.
		simulation.doStep();
		simulation.fastForwardUntillPredicate([&]()->bool { return blocks.solid_is(wallLocation2); }, 90);
		REQUIRE(actors.objective_getCurrentName(dwarf1) != L"construct");
		REQUIRE(actors.objective_getCurrentName(dwarf2) != L"construct");
		REQUIRE(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("make stairs")
	{
		BlockIndex stairsLocation = blocks.getIndex_i(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, stairsLocation, &BlockFeatureType::stairs, wood);
		REQUIRE(blocks.designation_has(stairsLocation, faction, BlockDesignation::Construct));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, stairsLocation));
		REQUIRE(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, stairsLocation);
		// Search for project and find stairs and reserve all required items.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Select a haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		// Find a path.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1) .exists());
		std::function<bool()> predicate = [&]() { return blocks.blockFeature_contains(stairsLocation, BlockFeatureType::stairs); };
		simulation.fastForwardUntillPredicate(predicate, 180);
		REQUIRE(blocks.blockFeature_contains(stairsLocation, BlockFeatureType::stairs));
	}
	SUBCASE("cannot path to spot")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 2), blocks.getIndex_i(8, 3, 2), wood);
		BlockIndex gateway = blocks.getIndex_i(9, 3, 2);
		BlockIndex wallLocation = blocks.getIndex_i(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project, and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		blocks.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != L"construct");
		REQUIRE(!items.reservable_isFullyReserved(saw, faction));
		REQUIRE(project.getWorkers().empty());
	}
	SUBCASE("spot has become solid")
	{
		BlockIndex wallLocation = blocks.getIndex_i(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		blocks.solid_set(wallLocation, wood, false);
		REQUIRE(!actors.project_exists(dwarf1));
		REQUIRE(!items.reservable_isFullyReserved(saw, faction));
		REQUIRE(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("saw destroyed")
	{
		BlockIndex wallLocation = blocks.getIndex_i(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Destroying the saw triggers a dishonor callback which resets the project.
		items.destroy(saw);
		REQUIRE(project.getWorkers().empty());
		REQUIRE(!project.reservationsComplete());
		REQUIRE(!project.hasCandidate(dwarf1));
	}
	SUBCASE("player cancels")
	{
		BlockIndex wallLocation = blocks.getIndex_i(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		area.m_hasConstructionDesignations.undesignate(faction, wallLocation);
		REQUIRE(!actors.project_exists(dwarf1));
		REQUIRE(!items.reservable_isFullyReserved(saw, faction));
		REQUIRE(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("player interrupts")
	{
		BlockIndex wallLocation = blocks.getIndex_i(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the tools and materials.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(blocks.getIndex_i(0, 8, 3));
		actors.objective_addTaskToStart(dwarf1, std::move(objective));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != L"construct");
		REQUIRE(!actors.project_exists(dwarf1));
		REQUIRE(!items.reservable_isFullyReserved(saw, faction));
		REQUIRE(area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("dirt wall")
	{
		ItemTypeId pile = ItemType::byName(L"pile");
		MaterialTypeId dirt = MaterialType::byName(L"dirt");
		ItemIndex dirtPile = items.create({.itemType=pile, .materialType=dirt, .location=blocks.getIndex_i(9, 9, 2), .quantity=Quantity::create(150u)});
		BlockIndex wallLocation = blocks.getIndex_i(3, 3, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		Quantity dirtPerLoad = actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(dirtPile), Config::minimumHaulSpeedInital);
		// One step to find the designation, activate the project and reserve the pile.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf1));
		REQUIRE(actors.getActionDescription(dwarf1) == L"construct");
		ConstructProject& project = static_cast<ConstructProject&>(*actors.project_get(dwarf1));
		REQUIRE(items.reservable_hasAnyReservations(dirtPile));
		REQUIRE(project.reservationsComplete());
		REQUIRE(project.getWorkers().contains(dwarf1Ref));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pile.
		simulation.doStep();
		auto& consumed = MaterialType::construction_getConsumed(dirt);
		auto found = std::ranges::find_if(consumed, [dirtPile, &area](auto pair) { return pair.first.query(area, dirtPile); });
		REQUIRE(found != consumed.end());
		Quantity quantity = found->second;
		REQUIRE(quantity <= items.getQuantity(dirtPile));
		Quantity trips = Quantity::create(std::ceil((float)quantity.get() / (float)dirtPerLoad.get()));
		for(uint i = 0; i < trips; ++i)
		{
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, items.getLocation(dirtPile));
			if(!actors.canPickUp_isCarryingItem(dwarf1, dirtPile))
				REQUIRE(actors.canPickUp_isCarryingItemGeneric(dwarf1, pile, dirt, dirtPerLoad));
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
			REQUIRE(!actors.canPickUp_exists(dwarf1));
			REQUIRE(actors.project_exists(dwarf1));
			if(!project.deliveriesComplete())
			{
				REQUIRE(project.hasTryToHaulThreadedTask());
				simulation.doStep();
				REQUIRE(actors.move_hasPathRequest(dwarf1));
			}
		}
		REQUIRE(actors.project_get(dwarf1) == &project);
		REQUIRE(actors.getActionDescription(dwarf1) == L"construct");
		REQUIRE(project.deliveriesComplete());
		REQUIRE(project.getFinishStep() != 0);
		simulation.fastForwardUntillPredicate([&]{ return blocks.solid_is(wallLocation); }, 130);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(pile) == 0);
	}
	SUBCASE("dirt wall three piles")
	{
		ItemTypeId pile = ItemType::byName(L"pile");
		MaterialTypeId dirt = MaterialType::byName(L"dirt");
		BlockIndex pileLocation1 = blocks.getIndex_i(9, 9, 2);
		BlockIndex pileLocation2 = blocks.getIndex_i(9, 8, 2);
		BlockIndex pileLocation3 = blocks.getIndex_i(9, 7, 2);
		ItemIndex dirtPile1 = items.create({.itemType=pile, .materialType=dirt, .location=pileLocation1, .quantity=Quantity::create(50u)});
		ItemIndex dirtPile2 = items.create({.itemType=pile, .materialType=dirt, .location=pileLocation2, .quantity=Quantity::create(50u)});
		ItemIndex dirtPile3 = items.create({.itemType=pile, .materialType=dirt, .location=pileLocation3, .quantity=Quantity::create(50u)});
		BlockIndex wallLocation = blocks.getIndex_i(9, 0, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation,activate the project and reserve the pile.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf1));
		REQUIRE(actors.getActionDescription(dwarf1) == L"construct");
		ConstructProject& project = static_cast<ConstructProject&>(*actors.project_get(dwarf1));
		REQUIRE(items.reservable_hasAnyReservations(dirtPile1));
		REQUIRE(items.reservable_hasAnyReservations(dirtPile2));
		REQUIRE(items.reservable_hasAnyReservations(dirtPile3));
		REQUIRE(project.reservationsComplete());
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to a pile.
		simulation.doStep();
		// Move pile 3.
		auto pile3Moved = [&]{ return blocks.item_empty(pileLocation3); };
		simulation.fastForwardUntillPredicate(pile3Moved);
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, dirtPile3));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		// Move pile 2.
		auto pile2Moved = [&]{ return blocks.item_empty(pileLocation2); };
		simulation.fastForwardUntillPredicate(pile2Moved);
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, dirtPile2));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		// Move pile 1.
		auto pile1Moved = [&]{ return blocks.item_empty(pileLocation1); };
		simulation.fastForwardUntillPredicate(pile1Moved);
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, dirtPile1));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
		REQUIRE(!actors.canPickUp_exists(dwarf1));
		REQUIRE(project.deliveriesComplete());
		REQUIRE(project.getFinishStep() != 0);
		auto predicate = [&]{ return blocks.solid_is(wallLocation); };
		simulation.fastForwardUntillPredicate(predicate, 130);
	}
	SUBCASE("dirt wall not enough dirt")
	{
		ItemTypeId pile = ItemType::byName(L"pile");
		MaterialTypeId dirt = MaterialType::byName(L"dirt");
		ItemIndex dirtPile = items.create({.itemType=pile, .materialType=dirt, .location=blocks.getIndex_i(9, 9, 2), .quantity=Quantity::create(140u)});
		BlockIndex wallLocation = blocks.getIndex_i(3, 3, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		// One step to find the designation but fail to activate
		simulation.doStep();
		REQUIRE(!items.reservable_hasAnyReservations(dirtPile));
		REQUIRE(!project.reservationsComplete());
		REQUIRE(project.getWorkers().empty());
		REQUIRE(!project.hasCandidate(dwarf1));
		REQUIRE(!actors.project_exists(dwarf1));
		ConstructObjective& objective = actors.objective_getCurrent<ConstructObjective>(dwarf1);
		REQUIRE(!objective.getProject());
		REQUIRE(!objective.joinableProjectExistsAt(area, wallLocation, dwarf1));
		// One step to fail to find another construction project.
		simulation.doStep();
		REQUIRE(!actors.project_exists(dwarf1));
		REQUIRE(actors.getActionDescription(dwarf1) != L"construct");
	}
	SUBCASE("dirt wall multiple small piles")
	{
		ItemTypeId pile = ItemType::byName(L"pile");
		MaterialTypeId dirt = MaterialType::byName(L"dirt");
		BlockIndex wallLocation = blocks.getIndex_i(3, 3, 2);
		Cuboid pileLocation(blocks, blocks.getIndex_i(9, 9, 2), blocks.getIndex_i(0, 9, 2));
		for(const BlockIndex& block : pileLocation.getView(blocks))
			items.create({.itemType=pile, .materialType=dirt, .location=block, .quantity=Quantity::create(15)});
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the piles.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf1));
		REQUIRE(actors.getActionDescription(dwarf1) == L"construct");
		ConstructProject& project = static_cast<ConstructProject&>(*actors.project_get(dwarf1));
		ItemIndex firstInBlock = blocks.item_getAll(blocks.getIndex_i(8,9,2)).front();
		REQUIRE(items.reservable_hasAnyReservations(firstInBlock));
		REQUIRE(project.reservationsComplete());
		// One step to pick a haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1Ref).haulSubproject);
		// One step to path.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1) .exists());
		auto predicate = [&]{ return blocks.solid_is(wallLocation); };
		simulation.fastForwardUntillPredicate(predicate, 130);
	}
}
