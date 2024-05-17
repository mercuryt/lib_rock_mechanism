#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/dig.h"
#include "../../engine/cuboid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/wander.h"
TEST_CASE("dig")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& bronze = MaterialType::byName("bronze");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
	areaBuilderUtil::setSolidLayers(area, 2, 3, dirt);
	Faction faction(L"tower of power");
	DigObjectiveType digObjectiveType;
	Actor& dwarf1 = simulation.m_hasActors->createActor(dwarf, area.getBlock(1, 1, 4));
	dwarf1.setFaction(&faction);
	Block& pickLocation = area.getBlock(5, 5, 4);
	Item& pick = simulation.m_hasItems->createItemNongeneric(ItemType::byName("pick"), bronze, 50u, 0);
	pick.setLocation(pickLocation);
	area.m_hasDigDesignations.addFaction(faction);
	SUBCASE("dig hole")
	{
		Block& holeLocation = area.getBlock(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.at(faction, holeLocation);
		REQUIRE(holeLocation.m_hasDesignations.contains(faction, BlockDesignation::Dig));
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
		REQUIRE(pick.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, *pick.m_location);
		REQUIRE(dwarf1.m_canPickup.isCarrying(pick));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, holeLocation);
		Step stepsDuration = project.getDuration();
		simulation.fastForward(stepsDuration);
		REQUIRE(!holeLocation.isSolid());
		REQUIRE(holeLocation.m_hasBlockFeatures.empty());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
	}
	SUBCASE("dig stairs and tunnel")
	{
		Block& aboveStairs = area.getBlock(8, 4, 4);
		Block& stairsLocation1 = area.getBlock(8, 4, 3);
		Block& stairsLocation2 = area.getBlock(8, 4, 2);
		Block& tunnelStart = area.getBlock(8, 5, 2);
		Block& tunnelEnd = area.getBlock(8, 6, 2);
		Cuboid tunnel(tunnelEnd, tunnelStart);
		area.m_hasDigDesignations.designate(faction, stairsLocation1, &BlockFeatureType::stairs);
		area.m_hasDigDesignations.designate(faction, stairsLocation2, &BlockFeatureType::stairs);
		for(Block& block : tunnel)
			area.m_hasDigDesignations.designate(faction, block, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !stairsLocation1.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 45);
		DigObjective& objective = static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent());
		REQUIRE(objective.name() == "dig");
		REQUIRE(objective.getProject() == nullptr);
		REQUIRE(stairsLocation1.m_hasBlockFeatures.contains(BlockFeatureType::stairs));
		REQUIRE(stairsLocation1.m_hasShapes.canEnterEverFrom(dwarf1, aboveStairs));
		REQUIRE(objective.hasThreadedTask());
		// Find next project.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == objective);
		REQUIRE(objective.getProject() != nullptr);
		REQUIRE(objective.getProject()->getLocation() == stairsLocation2);
		REQUIRE(dwarf1.m_canMove.getDestination() == nullptr);
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
		std::function<bool()> predicate2 = [&]() { return !stairsLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate2, 45);
		std::function<bool()> predicate3 = [&]() { return !tunnelEnd.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate3, 100);
	}
	SUBCASE("tunnel into cliff face with equipmentSet pick")
	{
		Block& cliffLowest = area.getBlock(7, 0, 4);
		Block& cliffHighest = area.getBlock(9, 9, 5);
		areaBuilderUtil::setSolidWall(cliffLowest, cliffHighest, dirt);
		Block& tunnelStart = area.getBlock(7, 5, 4);
		Block& tunnelEnd = area.getBlock(9, 5, 4);
		Block& tunnelSecond = area.getBlock(8, 5, 4);
		for(Block& block : Cuboid(tunnelEnd, tunnelStart))
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
		REQUIRE(dwarf1.m_canMove.getDestination());
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, tunnelStart);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !tunnelStart.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 45);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == dwarf1.m_hasObjectives.getCurrent());
		REQUIRE(!static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent()).getProject());
		REQUIRE(!tunnelStart.m_reservable.hasAnyReservations());
		// Find next project.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent() == dwarf1.m_hasObjectives.getCurrent());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		Project* project = static_cast<DigObjective&>(dwarf1.m_hasObjectives.getCurrent()).getProject();
		REQUIRE(project);
		REQUIRE(project->getLocation() == tunnelSecond);
		REQUIRE(!dwarf1.m_canMove.getDestination());
		REQUIRE(!tunnelStart.m_reservable.hasAnyReservations());
		// Make reservations and activate.
		simulation.doStep();
		REQUIRE(!tunnelStart.m_reservable.hasAnyReservations());
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
		std::function<bool()> predicate2 = [&]() { return !tunnelSecond.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate2, 45);
		std::function<bool()> predicate3 = [&]() { return !tunnelEnd.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate3, 45);
	}
	SUBCASE("two workers")
	{
		Actor& dwarf2 = simulation.m_hasActors->createActor(dwarf, area.getBlock(1, 2, 4));
		dwarf2.setFaction(&faction);
		Block& holeLocation = area.getBlock(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !holeLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 22);
	}
	SUBCASE("two workers two holes")
	{
		Actor& dwarf2 = simulation.m_hasActors->createActor(dwarf, area.getBlock(1, 2, 4));
		dwarf2.setFaction(&faction);
		Block& holeLocation1 = area.getBlock(6, 2, 3);
		Block& holeLocation2 = area.getBlock(9, 8, 3);
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
		std::function<bool()> predicate1 = [&]() { return !holeLocation1.isSolid(); };
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
		std::function<bool()> predicate2 = [&]() { return !holeLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate2, Step(22));
		simulation.doStep();
		REQUIRE(dwarf1.getActionDescription() != L"dig");
		REQUIRE(dwarf2.getActionDescription() != L"dig");
	}
	SUBCASE("cannot path to spot")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 3, 4), area.getBlock(8, 3, 4), dirt);
		Block& gateway = area.getBlock(9, 3, 4);
		Block& holeLocation = area.getBlock(8, 4, 3);
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
		gateway.setSolid(dirt);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(project.getWorkers().empty());
	}
	SUBCASE("spot already dug")
	{
		Block& holeLocation = area.getBlock(8, 8, 3);
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
		holeLocation.setNotSolid();
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(!area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("pick destroyed")
	{
		Block& holeLocation = area.getBlock(8, 4, 3);
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
		Block& holeLocation = area.getBlock(8, 8, 3);
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
		Block& holeLocation = area.getBlock(8, 8, 3);
		Block& goToLocation = area.getBlock(1, 8, 4);
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
		REQUIRE(dwarf1.m_canMove.getDestination()->isAdjacentTo(pick));
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, *pick.m_location);
		REQUIRE(dwarf1.m_canPickup.isCarrying(pick));
		auto holeDug = [&]{ return !holeLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(holeDug, 45);
	}
	SUBCASE("player interrupts worker, project may reset")
	{
		pick.exit();
		dwarf1.m_equipmentSet.addEquipment(pick);
		Actor& dwarf2 = simulation.m_hasActors->createActor(dwarf, area.getBlock(1, 2, 4));
		dwarf2.setFaction(&faction);
		Block& holeLocation = area.getBlock(8, 8, 3);
		Block& goToLocation = area.getBlock(1, 8, 4);
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
		auto holeDug = [&]{ return !holeLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(holeDug, 22);
		REQUIRE(!digObjectiveType.canBeAssigned(dwarf1));
		REQUIRE(!digObjectiveType.canBeAssigned(dwarf2));
	}
}
