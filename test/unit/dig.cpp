#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/dig.h"
#include "../../engine/cuboid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/wander.h"
#include "../../engine/objectives/dig.h"
#include "../../engine/itemType.h"
#include "../../engine/items/items.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/types.h"
#include "../../engine/animalSpecies.h"
TEST_CASE("dig")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& bronze = MaterialType::byName("bronze");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
	areaBuilderUtil::setSolidLayers(area, 2, 3, dirt);
	FactionId faction = simulation.createFaction(L"Tower Of Power").id;
	area.m_blockDesignations.registerFaction(faction);
	DigObjectiveType digObjectiveType;
	ActorIndex dwarf1 = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 4),
		.faction=faction,
	});
	ActorReference dwarf1Ref = dwarf1.toReference(area);
	BlockIndex pickLocation = blocks.getIndex_i(5, 5, 4);
	ItemIndex pick = items.create({.itemType=ItemType::byName("pick"), .materialType=bronze, .location=pickLocation, .quality=Quality::create(50u), .percentWear=Percent::create(0)});
	area.m_hasDigDesignations.addFaction(faction);
	SUBCASE("dig hole")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.at(faction, holeLocation);
		REQUIRE(blocks.designation_has(holeLocation, faction, BlockDesignation::Dig));
		REQUIRE(area.m_hasDigDesignations.contains(faction, holeLocation));
		REQUIRE(digObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		REQUIRE(actors.move_getPath(dwarf1).empty());
		REQUIRE(items.isAdjacentToLocation(pick, actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, items.getLocation(pick));
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, pick));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, holeLocation);
		Step stepsDuration = project.getDuration();
		simulation.fastForward(stepsDuration);
		REQUIRE(!blocks.solid_is(holeLocation));
		REQUIRE(blocks.blockFeature_empty(holeLocation));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "dig");
		REQUIRE(!actors.canPickUp_isCarryingItem(dwarf1, pick));
	}
	SUBCASE("dig stairs and tunnel")
	{
		BlockIndex aboveStairs = blocks.getIndex_i(8, 4, 4);
		BlockIndex stairsLocation1 = blocks.getIndex_i(8, 4, 3);
		BlockIndex stairsLocation2 = blocks.getIndex_i(8, 4, 2);
		BlockIndex tunnelStart = blocks.getIndex_i(8, 5, 2);
		BlockIndex tunnelEnd = blocks.getIndex_i(8, 6, 2);
		Cuboid tunnel(blocks, tunnelEnd, tunnelStart);
		area.m_hasDigDesignations.designate(faction, stairsLocation1, &BlockFeatureType::stairs);
		area.m_hasDigDesignations.designate(faction, stairsLocation2, &BlockFeatureType::stairs);
		for(BlockIndex block : tunnel)
			area.m_hasDigDesignations.designate(faction, block, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(stairsLocation1); };
		simulation.fastForwardUntillPredicate(predicate, 45);
		DigObjective& objective = actors.objective_getCurrent<DigObjective>(dwarf1);
		REQUIRE(objective.name() == "dig");
		REQUIRE(objective.getProject() == nullptr);
		REQUIRE(blocks.blockFeature_contains(stairsLocation1, BlockFeatureType::stairs));
		REQUIRE(blocks.shape_shapeAndMoveTypeCanEnterEverFrom(stairsLocation1, actors.getShape(dwarf1), actors.getMoveType(dwarf1), aboveStairs));
		REQUIRE(actors.move_hasPathRequest(dwarf1));
		// Find next project.
		simulation.doStep();
		REQUIRE(&actors.objective_getCurrent<DigObjective>(dwarf1) == &objective);
		REQUIRE(objective.getProject() != nullptr);
		REQUIRE(objective.getProject()->getLocation() == stairsLocation2);
		REQUIRE(actors.move_getDestination(dwarf1).empty());
		REQUIRE(!items.reservable_hasAnyReservations(pick));
		// Make reservations and activate.
		simulation.doStep();
		REQUIRE(objective.getProject() != nullptr);
		REQUIRE(items.reservable_hasAnyReservations(pick));
		// Select haul type to get pick to location.
		simulation.doStep();
		REQUIRE(objective.getProject() != nullptr);
		REQUIRE(objective.getProject()->getProjectWorkerFor(dwarf1Ref).haulSubproject);
		// Path to locaton.
		simulation.doStep();
		REQUIRE(&actors.objective_getCurrent<DigObjective>(dwarf1) == &objective);
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(stairsLocation2); };
		simulation.fastForwardUntillPredicate(predicate2, 45);
		std::function<bool()> predicate3 = [&]() { return !blocks.solid_is(tunnelEnd); };
		simulation.fastForwardUntillPredicate(predicate3, 100);
	}
	SUBCASE("tunnel into cliff face with equipmentSet pick")
	{
		BlockIndex cliffLowest = blocks.getIndex_i(7, 0, 4);
		BlockIndex cliffHighest = blocks.getIndex_i(9, 9, 5);
		areaBuilderUtil::setSolidWall(area, cliffLowest, cliffHighest, dirt);
		BlockIndex tunnelStart = blocks.getIndex_i(7, 5, 4);
		BlockIndex tunnelEnd = blocks.getIndex_i(9, 5, 4);
		BlockIndex tunnelSecond = blocks.getIndex_i(8, 5, 4);
		for(BlockIndex block : Cuboid(blocks, tunnelEnd, tunnelStart))
			area.m_hasDigDesignations.designate(faction, block, nullptr);
		items.exit(pick);
		actors.equipment_add(dwarf1, pick);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		// Find project.
		simulation.doStep();
		// Reserve.
		simulation.doStep();
		REQUIRE(items.reservable_isFullyReserved(pick, faction));
		REQUIRE(actors.project_get(dwarf1)->reservationsComplete());
		// Path to location.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, tunnelStart);
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(tunnelStart); };
		simulation.fastForwardUntillPredicate(predicate, 45);
		REQUIRE(!actors.objective_getCurrent<DigObjective>(dwarf1).getProject());
		REQUIRE(!blocks.isReserved(tunnelStart, faction));
		// Find next project.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		Project* project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		REQUIRE(project);
		REQUIRE(project->getLocation() == tunnelSecond);
		REQUIRE(!actors.move_getDestination(dwarf1).exists());
		REQUIRE(!blocks.isReserved(tunnelStart, faction));
		// Make reservations and activate.
		simulation.doStep();
		REQUIRE(!blocks.isReserved(tunnelStart, faction));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		REQUIRE(project);
		REQUIRE(project->reservationsComplete());
		REQUIRE(items.reservable_hasAnyReservations(pick));
		REQUIRE(actors.move_hasPathRequest(dwarf1));
		// Path to location.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		REQUIRE(project);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, tunnelSecond);
		project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		REQUIRE(project);
		REQUIRE(project->getLocation() == tunnelSecond);
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(tunnelSecond); };
		simulation.fastForwardUntillPredicate(predicate2, 45);
		std::function<bool()> predicate3 = [&]() { return !blocks.solid_is(tunnelEnd); };
		simulation.fastForwardUntillPredicate(predicate3, 45);
	}
	SUBCASE("two workers")
	{
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1, 2, 4),
			.faction=faction,
		});
		BlockIndex holeLocation = blocks.getIndex_i(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		actors.objective_setPriority(dwarf2, digObjectiveType, Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf2) == "dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(holeLocation); };
		simulation.fastForwardUntillPredicate(predicate, 22);
	}
	SUBCASE("two workers two holes")
	{
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1, 2, 4),
			.faction=faction,
		});
		BlockIndex holeLocation1 = blocks.getIndex_i(6, 2, 3);
		BlockIndex holeLocation2 = blocks.getIndex_i(9, 8, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation1, nullptr);
		area.m_hasDigDesignations.designate(faction, holeLocation2, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		actors.objective_setPriority(dwarf2, digObjectiveType, Priority::create(100));
		REQUIRE(actors.objective_getCurrentName(dwarf2) == "dig");
		// Workers find closer project first.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		REQUIRE(actors.getActionDescription(dwarf1) == L"dig");
		DigProject& project1 = static_cast<DigProject&>(*actors.project_get(dwarf1));
		REQUIRE(project1.getLocation() == holeLocation1);
		REQUIRE(actors.project_get(dwarf1) == actors.project_get(dwarf2));
		// Reserve project.
		simulation.doStep();
		REQUIRE(project1.reservationsComplete());
		// Wait for first project to complete.
		std::function<bool()> predicate1 = [&]() { return !blocks.solid_is(holeLocation1); };
		simulation.fastForwardUntillPredicate(predicate1, 22);
		REQUIRE(!items.reservable_hasAnyReservations(pick));
		// Workers find second project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		REQUIRE(actors.getActionDescription(dwarf1) == L"dig");
		DigProject& project2 = static_cast<DigProject&>(*actors.project_get(dwarf1));
		REQUIRE(project2.getLocation() == holeLocation2);
		REQUIRE(actors.project_get(dwarf1) == actors.project_get(dwarf2));
		// Reserve second Project.
		simulation.doStep();
		REQUIRE(project2.reservationsComplete());
		// Wait for second project to complete.
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(holeLocation2); };
		simulation.fastForwardUntillPredicate(predicate2, 22);
		simulation.doStep();
		REQUIRE(actors.getActionDescription(dwarf1) != L"dig");
		REQUIRE(actors.getActionDescription(dwarf2) != L"dig");
	}
	SUBCASE("cannot path to spot")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 4), blocks.getIndex_i(8, 3, 4), dirt);
		BlockIndex gateway = blocks.getIndex_i(9, 3, 4);
		BlockIndex holeLocation = blocks.getIndex_i(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.at(faction, holeLocation);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		blocks.solid_set(gateway, dirt, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "dig");
		REQUIRE(!items.reservable_isFullyReserved(pick, faction));
		REQUIRE(project.getWorkers().empty());
	}
	SUBCASE("spot already dug")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting holeLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		blocks.solid_setNot(holeLocation);
		REQUIRE(!actors.canPickUp_isCarryingItem(dwarf1, pick));
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		REQUIRE(!items.reservable_isFullyReserved(pick, faction));
		REQUIRE(!area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("pick destroyed")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.at(faction, holeLocation);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Destroying the pick triggers a dishonor callback which resets the project.
		items.destroy(pick);
		REQUIRE(project.getWorkers().empty());
		REQUIRE(!project.reservationsComplete());
		REQUIRE(!project.hasCandidate(dwarf1));
	}
	SUBCASE("player cancels")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting holeLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		area.m_hasDigDesignations.undesignate(faction, holeLocation);
		REQUIRE(!actors.canPickUp_isCarryingItem(dwarf1, pick));
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		REQUIRE(!items.reservable_isFullyReserved(pick, faction));
		REQUIRE(!area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("player interrupts")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		BlockIndex goToLocation = blocks.getIndex_i(1, 8, 4);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		// Usurp the current objective, delaying the project.
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(goToLocation);
		actors.objective_addTaskToStart(dwarf1, std::move(objective));
		REQUIRE(actors.objective_getCurrentName(dwarf1) != "dig");
		REQUIRE(!actors.canPickUp_isCarryingItem(dwarf1, pick));
		REQUIRE(actors.project_get(dwarf1) == nullptr);
		REQUIRE(!items.reservable_isFullyReserved(pick, faction));
		REQUIRE(project.getWorkersAndCandidates().empty());
		REQUIRE(area.m_hasDigDesignations.areThereAnyForFaction(faction));
		simulation.fastForwardUntillActorIsAt(area, dwarf1, goToLocation);
		REQUIRE(actors.objective_getCurrentName(dwarf1) == "dig");
		// One step to find the designation.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1) == &project);
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		REQUIRE(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1Ref).haulSubproject);
		// Another step to path to the pick.
		simulation.doStep();
		REQUIRE(items.isAdjacentToActor(pick, dwarf1));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, items.getLocation(pick));
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf1, pick));
		auto holeDug = [&]{ return !blocks.solid_is(holeLocation); };
		simulation.fastForwardUntillPredicate(holeDug, 45);
	}
	SUBCASE("player interrupts worker, project may reset")
	{
		items.exit(pick);
		actors.equipment_add(dwarf1, pick);
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=blocks.getIndex_i(1, 2, 4),
			.faction=faction,
		});
		ActorReference dwarf2Ref = dwarf2.toReference(area);
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		BlockIndex goToLocation = blocks.getIndex_i(1, 8, 4);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType, Priority::create(100));
		actors.objective_setPriority(dwarf2, digObjectiveType, Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to path to the project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		REQUIRE(actors.project_get(dwarf2) == &project);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, holeLocation);
		REQUIRE(project.finishEventExists());
		REQUIRE(project.reservationsComplete());
		REQUIRE(items.reservable_hasAnyReservations(pick));
		SUBCASE("reset")
		{
			// Usurp the current objective, reseting the project.
			std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(goToLocation);
			actors.objective_addTaskToStart(dwarf1, std::move(objective));
			REQUIRE(!project.reservationsComplete());
			REQUIRE(!items.reservable_hasAnyReservations(pick));
			REQUIRE(actors.project_get(dwarf2) == &project);
			// Project is unable to reserve.
			simulation.doStep();
			REQUIRE(!project.reservationsComplete());
			REQUIRE(!actors.project_get(dwarf2));
			REQUIRE(!actors.objective_getCurrent<DigObjective>(dwarf2).getJoinableProjectAt(area, holeLocation, dwarf2));
			// dwarf2 is unable to find another dig project, turns on objective type delay in objective priority set.
			simulation.doStep();
			REQUIRE(actors.objective_isOnDelay(dwarf2, ObjectiveTypeId::Dig));
			simulation.fastForwardUntillActorIsAt(area, dwarf1, goToLocation);
			REQUIRE(!actors.project_get(dwarf2));
			// One step to find the designation.
			simulation.doStep();
			REQUIRE(actors.project_get(dwarf1));
			// One step to activate the project and reserve the pick.
			simulation.doStep();
			REQUIRE(project.reservationsComplete());
			// One step to path to the project.
			simulation.doStep();
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, holeLocation);
			REQUIRE(actors.objective_isOnDelay(dwarf2, ObjectiveTypeId::Dig));
			Step dwarf2DelayEndsAt = actors.objective_getDelayEndFor(dwarf2, ObjectiveTypeId::Dig);
			Step delay = dwarf2DelayEndsAt - simulation.m_step;
			simulation.fastForward(delay);
			REQUIRE(!actors.objective_isOnDelay(dwarf2, ObjectiveTypeId::Dig));
		}
		SUBCASE("no reset")
		{
			std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(goToLocation);
			actors.objective_addTaskToStart(dwarf2, std::move(objective));
			REQUIRE(!actors.project_get(dwarf2));
			REQUIRE(!project.getWorkers().contains(dwarf2Ref));
			REQUIRE(project.reservationsComplete());
			REQUIRE(actors.project_get(dwarf1) == &project);
		}
		auto holeDug = [&]{ return !blocks.solid_is(holeLocation); };
		simulation.fastForwardUntillPredicate(holeDug, 22);
		REQUIRE(!digObjectiveType.canBeAssigned(area, dwarf1));
		REQUIRE(!digObjectiveType.canBeAssigned(area, dwarf2));
	}
}
