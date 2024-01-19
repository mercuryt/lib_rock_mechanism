#include "../../lib/doctest.h"
#include "../../engine/actor.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/construct.h"
#include "../../engine/cuboid.h"
#include "../../engine/goTo.h"
TEST_CASE("construct")
{
	static const MaterialType& wood = MaterialType::byName("poplar wood");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
	Faction faction(L"tower of power");
	ConstructObjectiveType constructObjectiveType;
	Actor& dwarf1 = simulation.createActor(dwarf, area.getBlock(1, 1, 2));
	dwarf1.setFaction(&faction);
	area.m_hasConstructionDesignations.addFaction(faction);
	Item& boards = simulation.createItemGeneric(ItemType::byName("board"), wood, 50u);
	boards.setLocation(area.getBlock(8, 7, 2));
	Item& pegs = simulation.createItemGeneric(ItemType::byName("peg"), wood, 50u);
	pegs.setLocation(area.getBlock(3, 8, 2));
	Item& saw = simulation.createItemNongeneric(ItemType::byName("saw"), MaterialType::byName("bronze"), 25u, 0);
	saw.setLocation(area.getBlock(5, 7, 2));
	Item& mallet = simulation.createItemNongeneric(ItemType::byName("mallet"), wood, 25u, 0);
	mallet.setLocation(area.getBlock(9, 5, 2));
	SUBCASE("make wall")
	{
		Block& wallLocation = area.getBlock(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		REQUIRE(wallLocation.m_hasDesignations.contains(faction, BlockDesignation::Construct));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation));
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		// Search for accessable project.
		simulation.doStep();
		REQUIRE(project.hasCandidate(dwarf1));
		// Activete project and reserve all required.
		simulation.doStep();
		REQUIRE(dwarf1.m_project == &project);
		REQUIRE(project.reservationsComplete());
		// Select a haul strategy and create a subproject.
		simulation.doStep();
		// Find a path.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		std::function<bool()> predicate = [&]() { return wallLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 180);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(!area.m_hasConstructionDesignations.contains(faction, wallLocation));
		REQUIRE(!constructObjectiveType.canBeAssigned(dwarf1));
		REQUIRE(!wallLocation.m_hasDesignations.contains(faction, BlockDesignation::Construct));
		REQUIRE(boards.m_reservable.getUnreservedCount(faction) == boards.getQuantity());
		REQUIRE(pegs.m_reservable.getUnreservedCount(faction) == pegs.getQuantity());
		REQUIRE(!saw.m_reservable.isFullyReserved(&faction));
		REQUIRE(!mallet.m_reservable.isFullyReserved(&faction));
		REQUIRE(!wallLocation.m_reservable.isFullyReserved(&faction));
	}
	SUBCASE("make two walls")
	{
		Block& wallLocation1 = area.getBlock(8, 4, 2);
		Block& wallLocation2 = area.getBlock(8, 5, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, nullptr, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		// Find project to join.
		std::function<bool()> predicate = [&]() { return wallLocation1.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 180);
		predicate = [&]() { return wallLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 180);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
	}
	SUBCASE("make wall with two workers")
	{
		Actor& dwarf2 = simulation.createActor(dwarf, area.getBlock(1, 4, 2));
		dwarf2.setFaction(&faction);
		Block& wallLocation = area.getBlock(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "construct");
		// Find project to join.
		std::function<bool()> predicate = [&]() { return wallLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 90);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "construct");
	}
	SUBCASE("make two walls with two workers")
	{
		Actor& dwarf2 = simulation.createActor(dwarf, area.getBlock(1, 4, 2));
		dwarf2.setFaction(&faction);
		Block& wallLocation1 = area.getBlock(8, 4, 2);
		Block& wallLocation2 = area.getBlock(8, 5, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, nullptr, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "construct");
		ConstructProject& project1 = area.m_hasConstructionDesignations.getProject(faction, wallLocation1);
		ConstructProject& project2 = area.m_hasConstructionDesignations.getProject(faction, wallLocation2);
		// Find projects to join.
		simulation.doStep();
		REQUIRE(project1.hasCandidate(dwarf1));
		REQUIRE(project2.hasCandidate(dwarf2));
		// Activate project1 with dwarf1 and reserve all required, dwarf2 fails to validate project2 due to tools being reserved for project1.
		simulation.doStep();
		REQUIRE(dwarf1.m_project == &project1);
		REQUIRE(dwarf2.m_project == &project2);
		REQUIRE(project1.reservationsComplete());
		REQUIRE(!project2.reservationsComplete());
		REQUIRE(project1.getWorkers().size() == 1);
		REQUIRE(project2.getWorkers().empty());
		REQUIRE(!project1.hasCandidate(dwarf1));
		REQUIRE(project2.hasCandidate(dwarf2));
		// Select a haul strategy and create a subproject for dwarf1, dwarf2 tries again to activate project2, this time failing to find required unreserved items and activating delay.
		simulation.doStep();
		ProjectWorker& projectWorker1 = project1.getProjectWorkerFor(dwarf1);
		REQUIRE(projectWorker1.haulSubproject != nullptr);
		REQUIRE(project2.isOnDelay());
		REQUIRE(!project2.getLocation().m_hasDesignations.contains(faction, BlockDesignation::Construct));
		REQUIRE(project1.getWorkers().size() == 1);
		REQUIRE(!project2.hasCandidate(dwarf2));
		REQUIRE(dwarf2.m_project == nullptr);
		// Find a path for dwarf1, dwarf2 seeks project to join, and finds project1.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		REQUIRE(project1.hasCandidate(dwarf2));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation1));
		std::function<bool()> predicate = [&]() { return wallLocation1.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 90);
		REQUIRE(!area.m_hasConstructionDesignations.contains(faction, wallLocation1));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation2));
		REQUIRE(!project2.isOnDelay());
		REQUIRE(project2.getLocation().m_hasDesignations.contains(faction, BlockDesignation::Construct));
		// Both dwarves seek a project to join and find project2. This does not require a step if they are already adjacent but does if they are not.
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "construct");
		simulation.doStep();
		// Project2 completes reservations and both dwarfs graduate from candidate to worker.
		simulation.doStep();
		REQUIRE(project2.getWorkers().contains(&dwarf1));
		REQUIRE(project2.getWorkers().contains(&dwarf2));
		REQUIRE(!project2.hasCandidate(dwarf1));
		REQUIRE(!project2.hasCandidate(dwarf2));
		// Try to set haul strategy, both dwarves try to generate a haul subproject.
		simulation.doStep();
		predicate = [&]() { return wallLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, 90);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("make stairs")
	{
		Block& stairsLocation = area.getBlock(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, stairsLocation, &BlockFeatureType::stairs, wood);
		REQUIRE(stairsLocation.m_hasDesignations.contains(faction, BlockDesignation::Construct));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, stairsLocation));
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, stairsLocation);
		// Search for project and find stairs.
		simulation.doStep();
		REQUIRE(project.hasCandidate(dwarf1));
		// Reserve all required items.
		simulation.doStep();
		REQUIRE(project.reservationsComplete());
		// Select a haul strategy.
		simulation.doStep();
		REQUIRE(project.getProjectWorkerFor(dwarf1).haulSubproject != nullptr);
		// Find a path.
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		std::function<bool()> predicate = [&]() { return stairsLocation.m_hasBlockFeatures.contains(BlockFeatureType::stairs); };
		simulation.fastForwardUntillPredicate(predicate, 180);
		REQUIRE(stairsLocation.m_hasBlockFeatures.contains(BlockFeatureType::stairs));
	}
	SUBCASE("cannot path to spot")
	{
		areaBuilderUtil::setSolidWall(area.getBlock(0, 3, 2), area.getBlock(8, 3, 2), wood);
		Block& gateway = area.getBlock(9, 3, 2);
		Block& wallLocation = area.getBlock(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		gateway.setSolid(wood);
		simulation.fastForwardUntillActorIsAdjacentTo(dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(!saw.m_reservable.isFullyReserved(&faction));
		REQUIRE(project.getWorkers().empty());
	}
	SUBCASE("spot has become solid")
	{
		Block& wallLocation = area.getBlock(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		wallLocation.setSolid(wood);
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!saw.m_reservable.isFullyReserved(&faction));
		REQUIRE(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("saw destroyed")
	{
		Block& wallLocation = area.getBlock(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Destroying the saw triggers a dishonor callback which resets the project.
		saw.destroy();
		REQUIRE(project.getWorkers().empty());
		REQUIRE(!project.reservationsComplete());
		REQUIRE(!project.hasCandidate(dwarf1));
	}
	SUBCASE("player cancels")
	{
		Block& wallLocation = area.getBlock(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		area.m_hasConstructionDesignations.undesignate(faction, wallLocation);
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!saw.m_reservable.isFullyReserved(&faction));
		REQUIRE(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("player interrupts")
	{
		Block& wallLocation = area.getBlock(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(dwarf1, area.getBlock(0, 8, 3));
		dwarf1.m_hasObjectives.addTaskToStart(std::move(objective));
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(dwarf1.m_project == nullptr);
		REQUIRE(!saw.m_reservable.isFullyReserved(&faction));
		REQUIRE(area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
}
