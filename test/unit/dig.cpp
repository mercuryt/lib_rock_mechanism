#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/dig.h"
#include "../../engine/cuboid.h"
#include "../../engine/objectives/goTo.h"
#include "haul.h"
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
	Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(1, 1, 4));
	dwarf1.setFaction(&faction);
	Block& pickLocation = area.getBlock(5, 5, 4);
	Item& pick = simulation.createItemNongeneric(ItemType::byName("pick"), bronze, 50u, 0);
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
		Actor& dwarf2 = simulation.createActor(dwarf, area.getBlock(1, 2, 4));
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
		Actor& dwarf2 = simulation.createActor(dwarf, area.getBlock(1, 2, 4));
		dwarf2.setFaction(&faction);
		Block& holeLocation1 = area.getBlock(8, 4, 3);
		Block& holeLocation2 = area.getBlock(8, 3, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation1, nullptr);
		area.m_hasDigDesignations.designate(faction, holeLocation2, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !holeLocation1.isSolid() && !holeLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, Step(45));
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
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, area.getBlock(1, 8, 4));
		dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!pick.m_reservable.isFullyReserved(&faction));
		REQUIRE(area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
}
