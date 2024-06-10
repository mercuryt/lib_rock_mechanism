#include "../../lib/doctest.h"
#include "../../engine/actor.h"
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
#include "types.h"
TEST_CASE("dig")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& bronze = MaterialType::byName("bronze");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
	areaBuilderUtil::setSolidLayers(area, 2, 3, dirt);
	Faction faction(L"tower of power");
	area.m_blockDesignations.registerFaction(faction);
	DigObjectiveType digObjectiveType;
	Actor& dwarf1 = simulation.m_hasActors->createActor({
		.species=dwarf,
		.location=blocks.getIndex({1, 1, 4}),
		.area=&area,
	});
	dwarf1.setFaction(&faction);
	BlockIndex pickLocation = blocks.getIndex({5, 5, 4});
	Item& pick = simulation.m_hasItems->createItemNongeneric(ItemType::byName("pick"), bronze, 50u, 0);
	pick.setLocation(pickLocation, &area);
	area.m_hasDigDesignations.addFaction(faction);
	SUBCASE("dig hole")
	{
		BlockIndex holeLocation = blocks.getIndex({8, 4, 3});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.at(faction, holeLocation);
		REQUIRE(blocks.designation_has(holeLocation, faction, BlockDesignation::Dig));
		REQUIRE(area.m_hasDigDesignations.contains(faction, holeLocation));
		REQUIRE(digObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		REQUIRE(!dwarf1.m_canMove.getPath().empty());
		REQUIRE(pick.isAdjacentTo(dwarf1.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, pick.m_location);
		REQUIRE(dwarf1.m_canPickup.isCarrying(pick));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, holeLocation);
		Step stepsDuration = project.getDuration();
		simulation.fastForward(stepsDuration);
		REQUIRE(!blocks.solid_is(holeLocation));
		REQUIRE(blocks.blockFeature_empty(holeLocation));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
	}
	SUBCASE("dig stairs and tunnel")
	{
		BlockIndex aboveStairs = blocks.getIndex({8, 4, 4});
		BlockIndex stairsLocation1 = blocks.getIndex({8, 4, 3});
		BlockIndex stairsLocation2 = blocks.getIndex({8, 4, 2});
		BlockIndex tunnelStart = blocks.getIndex({8, 5, 2});
		BlockIndex tunnelEnd = blocks.getIndex({8, 6, 2});
		Cuboid tunnel(blocks, tunnelEnd, tunnelStart);
		area.m_hasDigDesignations.designate(faction, stairsLocation1, &BlockFeatureType::stairs);
		area.m_hasDigDesignations.designate(faction, stairsLocation2, &BlockFeatureType::stairs);
		for(BlockIndex block : tunnel)
			area.m_hasDigDesignations.designate(faction, block, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(stairsLocation1); };
		simulation.fastForwardUntillPredicate(predicate, 45);
		DigObjective& objective = static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent());
		REQUIRE(objective.name() == "dig");
		REQUIRE(objective.getProject() == nullptr);
		REQUIRE(blocks.blockFeature_contains(stairsLocation1, BlockFeatureType::stairs));
		REQUIRE(blocks.shape_canEnterEverFrom(stairsLocation1, dwarf1, aboveStairs));
		REQUIRE(objective.hasThreadedTask());
		// Find next project.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == objective);
		REQUIRE(objective.getProject() != nullptr);
		REQUIRE(objective.getProject()->getLocation() == stairsLocation2);
		REQUIRE(dwarf1.m_canMove.getDestination() == BLOCK_INDEX_MAX);
		REQUIRE(!pick.m_reservable.hasAnyReservations());
		// Make reservations and activate.
		simulation.doStep();
		REQUIRE(objective.getProject() != nullptr);
		REQUIRE(pick.m_reservable.hasAnyReservations());
		// Select haul type to get pick to location.
		simulation.doStep();
		REQUIRE(objective.getProject() != nullptr);
		REQUIRE(objective.getProject()->getProjectWorkerFor(dwarf1).haulSubproject);
		// Path to locaton.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == objective);
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(stairsLocation2); };
		simulation.fastForwardUntillPredicate(predicate2, 45);
		std::function<bool()> predicate3 = [&]() { return !blocks.solid_is(tunnelEnd); };
		simulation.fastForwardUntillPredicate(predicate3, 100);
	}
	SUBCASE("tunnel into cliff face with equipmentSet pick")
	{
		BlockIndex cliffLowest = blocks.getIndex({7, 0, 4});
		BlockIndex cliffHighest = blocks.getIndex({9, 9, 5});
		areaBuilderUtil::setSolidWall(area, cliffLowest, cliffHighest, dirt);
		BlockIndex tunnelStart = blocks.getIndex({7, 5, 4});
		BlockIndex tunnelEnd = blocks.getIndex({9, 5, 4});
		BlockIndex tunnelSecond = blocks.getIndex({8, 5, 4});
		for(BlockIndex block : Cuboid(blocks, tunnelEnd, tunnelStart))
			area.m_hasDigDesignations.designate(faction, block, nullptr);
		pick.exit();
		dwarf1.m_equipmentSet.addEquipment(pick);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		// Find project.
		simulation.doStep();
		// Reserve.
		simulation.doStep();
		REQUIRE(pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(dwarf1.m_project->reservationsComplete());
		// Path to location.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, tunnelStart);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(tunnelStart); };
		simulation.fastForwardUntillPredicate(predicate, 45);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == dwarf1.m_hasObjectives.getCurrent());
		REQUIRE(!static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent()).getProject());
		REQUIRE(!blocks.isReserved(tunnelStart, faction));
		// Find next project.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == dwarf1.m_hasObjectives.getCurrent());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		Project* project = static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent()).getProject();
		REQUIRE(project);
		REQUIRE(project->getLocation() == tunnelSecond);
		REQUIRE(!dwarf1.m_canMove.getDestination() != BLOCK_INDEX_MAX);
		REQUIRE(!blocks.isReserved(tunnelStart, faction));
		// Make reservations and activate.
		simulation.doStep();
		REQUIRE(!blocks.isReserved(tunnelStart, faction));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		project = static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent()).getProject();
		REQUIRE(project);
		REQUIRE(project->reservationsComplete());
		REQUIRE(pick.m_reservable.hasAnyReservations());
		REQUIRE(dwarf1.m_canMove.hasThreadedTask());
		// Path to location.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == dwarf1.m_hasObjectives.getCurrent());
		project = static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent()).getProject();
		REQUIRE(project);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, tunnelSecond);
		project = static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent()).getProject();
		REQUIRE(project);
		REQUIRE(project->getLocation() == tunnelSecond);
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(tunnelSecond); };
		simulation.fastForwardUntillPredicate(predicate2, 45);
		std::function<bool()> predicate3 = [&]() { return !blocks.solid_is(tunnelEnd); };
		simulation.fastForwardUntillPredicate(predicate3, 45);
	}
	SUBCASE("two workers")
	{
		Actor& dwarf2 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex({1, 2, 4}),
			.area=&area,
		});
		dwarf2.setFaction(&faction);
		BlockIndex holeLocation = blocks.getIndex({8, 4, 3});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(holeLocation); };
		simulation.fastForwardUntillPredicate(predicate, 22);
	}
	SUBCASE("two workers two holes")
	{
		Actor& dwarf2 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex({1, 2, 4}),
			.area=&area,
		});
		dwarf2.setFaction(&faction);
		BlockIndex holeLocation1 = blocks.getIndex({6, 2, 3});
		BlockIndex holeLocation2 = blocks.getIndex({9, 8, 3});
		area.m_hasDigDesignations.designate(faction, holeLocation1, nullptr);
		area.m_hasDigDesignations.designate(faction, holeLocation2, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "dig");
		// Workers find closer project first.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		REQUIRE(dwarf1.getActionDescription() == L"dig");
		DigProject& project1 = static_cast<DigProject&>(*dwarf1.m_project);
		REQUIRE(project1.getLocation() == holeLocation1);
		REQUIRE(dwarf1.m_project == dwarf2.m_project);
		// Reserve project.
		simulation.doStep();
		REQUIRE(project1.reservationsComplete());
		// Wait for first project to complete.
		std::function<bool()> predicate1 = [&]() { return !blocks.solid_is(holeLocation1); };
		simulation.fastForwardUntillPredicate(predicate1, Step(22));
		REQUIRE(!pick.m_reservable.hasAnyReservations());
		// Workers find second project.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		REQUIRE(dwarf1.getActionDescription() == L"dig");
		DigProject& project2 = static_cast<DigProject&>(*dwarf1.m_project);
		REQUIRE(project2.getLocation() == holeLocation2);
		REQUIRE(dwarf1.m_project == dwarf2.m_project);
		// Reserve second Project.
		simulation.doStep();
		REQUIRE(project2.reservationsComplete());
		// Wait for second project to complete.
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(holeLocation2); };
		simulation.fastForwardUntillPredicate(predicate2, Step(22));
		simulation.doStep();
		REQUIRE(dwarf1.getActionDescription() != L"dig");
		REQUIRE(dwarf2.getActionDescription() != L"dig");
	}
	SUBCASE("cannot path to spot")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex({0, 3, 4}), blocks.getIndex({8, 3, 4}), dirt);
		BlockIndex gateway = blocks.getIndex({9, 3, 4});
		BlockIndex holeLocation = blocks.getIndex({8, 4, 3});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.at(faction, holeLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(pick.m_reservable.isFullyReserved(&faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		blocks.solid_set(gateway, dirt, false);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(project.getWorkers().empty());
	}
	SUBCASE("spot already dug")
	{
		BlockIndex holeLocation = blocks.getIndex({8, 8, 3});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(pick.m_reservable.isFullyReserved(&faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting holeLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		blocks.solid_setNot(holeLocation);
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(!area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("pick destroyed")
	{
		BlockIndex holeLocation = blocks.getIndex({8, 4, 3});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.at(faction, holeLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(pick.m_reservable.isFullyReserved(&faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Destroying the pick triggers a dishonor callback which resets the project.
		pick.destroy();
		REQUIRE(project.getWorkers().empty());
		REQUIRE(!project.reservationsComplete());
		REQUIRE(!project.hasCandidate(dwarf1));
	}
	SUBCASE("player cancels")
	{
		BlockIndex holeLocation = blocks.getIndex({8, 8, 3});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
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
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(!area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("player interrupts")
	{
		BlockIndex holeLocation = blocks.getIndex({8, 8, 3});
		BlockIndex goToLocation = blocks.getIndex({1, 8, 4});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		Project& project = *dwarf1.m_project;
		// Usurp the current objective, delaying the project.
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, goToLocation);
		dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(project.getWorkersAndCandidates().empty());
		REQUIRE(area.m_hasDigDesignations.areThereAnyForFaction(faction));
		simulation.fastForwardUntillActorIsAt(dwarf1, goToLocation);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		// One step to find the designation.
		simulation.doStep();
		REQUIRE(dwarf1.m_project == &project);
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		REQUIRE(pick.m_reservable.isFullyReserved(&faction));
		// One step to select haul type.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject);
		// Another step to path to the pick.
		simulation.doStep();
		REQUIRE(pick.isAdjacentTo(dwarf1.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, pick.m_location);
		REQUIRE(dwarf1.m_canPickup.isCarrying(pick));
		auto holeDug = [&]{ return !blocks.solid_is(holeLocation); };
		simulation.fastForwardUntillPredicate(holeDug, 45);
	}
	SUBCASE("player interrupts worker, project may reset")
	{
		pick.exit();
		dwarf1.m_equipmentSet.addEquipment(pick);
		Actor& dwarf2 = simulation.m_hasActors->createActor({
			.species=dwarf,
			.location=blocks.getIndex({1, 2, 4}),
			.area=&area,
		});
		dwarf2.setFaction(&faction);
		BlockIndex holeLocation = blocks.getIndex({8, 8, 3});
		BlockIndex goToLocation = blocks.getIndex({1, 8, 4});
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to path to the project.
		simulation.doStep();
		REQUIRE(dwarf1.m_project);
		Project& project = *dwarf1.m_project;
		REQUIRE(dwarf2.m_project == &project);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, holeLocation);
		REQUIRE(project.finishEventExists());
		REQUIRE(project.reservationsComplete());
		REQUIRE(pick.m_reservable.hasAnyReservations());
		SUBCASE("reset")
		{
			// Usurp the current objective, reseting the project.
			std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, goToLocation);
			dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
			REQUIRE(!project.reservationsComplete());
			REQUIRE(!pick.m_reservable.hasAnyReservations());
			REQUIRE(dwarf2.m_project == &project);
			// Project is unable to reserve.
			simulation.doStep();
			REQUIRE(!project.reservationsComplete());
			REQUIRE(!dwarf2.m_project);
			REQUIRE(!static_cast<DigObjective&>(dwarf2.m_hasObjectives.getCurrent()).getJoinableProjectAt(holeLocation));
			// dwarf2 is unable to find another dig project, turns on objective type delay in objective priority set.
			simulation.doStep();
			REQUIRE(dwarf2.m_hasObjectives.m_prioritySet.isOnDelay(ObjectiveTypeId::Dig));
			simulation.fastForwardUntillActorIsAt(dwarf1, goToLocation);
			REQUIRE(!dwarf2.m_project);
			// One step to find the designation.
			simulation.doStep();
			REQUIRE(dwarf1.m_project);
			// One step to activate the project and reserve the pick.
			simulation.doStep();
			REQUIRE(project.reservationsComplete());
			// One step to path to the project.
			simulation.doStep();
			simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, holeLocation);
			REQUIRE(dwarf2.m_hasObjectives.m_prioritySet.isOnDelay(ObjectiveTypeId::Dig));
			Step dwarf2DelayEndsAt = dwarf2.m_hasObjectives.m_prioritySet.getDelayEndFor(ObjectiveTypeId::Dig);
			Step delay = dwarf2DelayEndsAt - simulation.m_step;
			simulation.fastForward(delay);
			REQUIRE(!dwarf2.m_hasObjectives.m_prioritySet.isOnDelay(ObjectiveTypeId::Dig));
		}
		SUBCASE("no reset")
		{
			std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf2, goToLocation);
			dwarf2.m_hasObjectives.addTaskToStart(std::move(objective));
			REQUIRE(!dwarf2.m_project);
			REQUIRE(!project.getWorkers().contains(&dwarf2));
			REQUIRE(project.reservationsComplete());
			REQUIRE(dwarf1.m_project == &project);
		}
		auto holeDug = [&]{ return !blocks.solid_is(holeLocation); };
		simulation.fastForwardUntillPredicate(holeDug, 22);
		REQUIRE(!digObjectiveType.canBeAssigned(dwarf1));
		REQUIRE(!digObjectiveType.canBeAssigned(dwarf2));
	}
}
