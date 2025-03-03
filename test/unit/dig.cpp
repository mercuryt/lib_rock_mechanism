#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/dig.h"
#include "../../engine/geometry/cuboid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/wander.h"
#include "../../engine/objectives/dig.h"
#include "../../engine/itemType.h"
#include "../../engine/items/items.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/types.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/portables.hpp"
TEST_CASE("dig")
{
	static MaterialTypeId dirt = MaterialType::byName(L"dirt");
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static MaterialTypeId bronze = MaterialType::byName(L"bronze");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Items& items = area.getItems();
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
	areaBuilderUtil::setSolidLayers(area, 2, 3, dirt);
	FactionId faction = simulation.createFaction(L"Tower of Power");
	area.m_blockDesignations.registerFaction(faction);
	const DigObjectiveType& digObjectiveType = static_cast<const DigObjectiveType&>(ObjectiveType::getByName(L"dig"));
	ActorIndex dwarf1 = actors.create({
		.species=dwarf,
		.location=blocks.getIndex_i(1, 1, 4),
		.faction=faction,
	});
	ActorReference dwarf1Ref = actors.m_referenceData.getReference(dwarf1);
	BlockIndex pickLocation = blocks.getIndex_i(5, 5, 4);
	ItemIndex pick = items.create({.itemType=ItemType::byName(L"pick"), .materialType=bronze, .location=pickLocation, .quality=Quality::create(50u), .percentWear=Percent::create(0)});
	area.m_hasDigDesignations.addFaction(faction);
	SUBCASE("dig hole")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.getForFactionAndBlock(faction, holeLocation);
		CHECK(blocks.designation_has(holeLocation, faction, BlockDesignation::Dig));
		CHECK(area.m_hasDigDesignations.contains(faction, holeLocation));
		CHECK(digObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		// One step to find the designation, activate the project, and reserve the pick.
		simulation.doStep();
		CHECK(project.reservationsComplete());
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		CHECK(actors.move_hasPathRequest(dwarf1));
		simulation.doStep();
		CHECK(!actors.move_getPath(dwarf1).empty());
		CHECK(items.isAdjacentToLocation(pick, actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, items.getLocation(pick));
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, pick));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, holeLocation);
		Step stepsDuration = project.getDuration();
		simulation.fastForward(stepsDuration);
		CHECK(!blocks.solid_is(holeLocation));
		CHECK(blocks.blockFeature_empty(holeLocation));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"dig");
		CHECK(!actors.canPickUp_isCarryingItem(dwarf1, pick));
		CHECK(!items.reservable_hasAnyReservations(pick));
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
		for(const BlockIndex& block : tunnel.getView(blocks))
			area.m_hasDigDesignations.designate(faction, block, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		// Find project, reserve, and activate.
		simulation.doStep();
		DigProject& project = *static_cast<DigProject*>(blocks.project_get(stairsLocation1, faction));
		CHECK(project.reservationsComplete());
		CHECK(project.getWorkers().contains(dwarf1Ref));
		// Select a haul type to take the pick to the project site.
		simulation.doStep();
		CHECK(actors.move_hasPathRequest(dwarf1));
		// Path to pick.
		simulation.doStep();
		CHECK(blocks.isAdjacentToIncludingCornersAndEdges(pickLocation, actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, pickLocation);
		CHECK(actors.move_hasPathRequest(dwarf1));
		// Path to project.
		simulation.doStep();
		CHECK(blocks.isAdjacentToIncludingCornersAndEdges(stairsLocation1, actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf1, stairsLocation1);
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(stairsLocation1); };
		simulation.fastForwardUntillPredicate(predicate, 180);
		actors.satisfyNeeds(dwarf1);
		DigObjective& objective = actors.objective_getCurrent<DigObjective>(dwarf1);
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		CHECK(objective.getProject() == nullptr);
		CHECK(blocks.blockFeature_contains(stairsLocation1, BlockFeatureType::stairs));
		CHECK(blocks.shape_shapeAndMoveTypeCanEnterEverFrom(stairsLocation1, actors.getShape(dwarf1), actors.getMoveType(dwarf1), aboveStairs));
		const auto& terrainFacade = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(dwarf1));
		CHECK(terrainFacade.accessable(aboveStairs, actors.getFacing(dwarf1), stairsLocation1, dwarf1));
		CHECK(actors.move_hasPathRequest(dwarf1));
		CHECK(!blocks.isReserved(stairsLocation1, faction));
		// Find next project, make reservations, and activate.
		simulation.doStep();
		CHECK(&actors.objective_getCurrent<DigObjective>(dwarf1) == &objective);
		CHECK(actors.move_getDestination(dwarf1).empty());
		CHECK(objective.getProject() != nullptr);
		CHECK(items.reservable_hasAnyReservations(pick));
		// Select haul type to get pick to location.
		simulation.doStep();
		CHECK(objective.getProject() != nullptr);
		// Path to locaton.
		simulation.doStep();
		CHECK(&actors.objective_getCurrent<DigObjective>(dwarf1) == &objective);
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(tunnelStart); };
		simulation.fastForwardUntillPredicate(predicate2, 180);
		actors.satisfyNeeds(dwarf1);
		// We are already adjacent to the next target, no need to path.
		DigProject& project2 = *static_cast<DigProject*>(blocks.project_get(stairsLocation2, faction));
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		DigObjective& objective2 = actors.objective_getCurrent<DigObjective>(dwarf1);
		CHECK(objective2.getProject() != nullptr);
		CHECK(objective2.getProject() == &project2);
		// Aready adjacent, no pathing required.
		CHECK(!actors.move_hasPathRequest(dwarf1));
		CHECK(!project2.reservationsComplete());
		simulation.doStep();
		CHECK(project2.reservationsComplete());
		CHECK(!actors.move_hasPathRequest(dwarf1));
		std::function<bool()> predicate3 = [&]() { return !blocks.solid_is(stairsLocation2); };
		simulation.fastForwardUntillPredicate(predicate3, 180);
		actors.satisfyNeeds(dwarf1);
		CHECK(objective.name() == L"dig");
		std::function<bool()> predicate4 = [&]() { return !blocks.solid_is(tunnelEnd); };
		simulation.fastForwardUntillPredicate(predicate4, 600);
	}
	SUBCASE("tunnel into cliff face with equipmentSet pick")
	{
		BlockIndex cliffLowest = blocks.getIndex_i(7, 0, 4);
		BlockIndex cliffHighest = blocks.getIndex_i(9, 9, 5);
		areaBuilderUtil::setSolidWall(area, cliffLowest, cliffHighest, dirt);
		BlockIndex tunnelStart = blocks.getIndex_i(7, 5, 4);
		BlockIndex tunnelEnd = blocks.getIndex_i(9, 5, 4);
		BlockIndex tunnelSecond = blocks.getIndex_i(8, 5, 4);
		Cuboid tunnel(blocks, tunnelEnd, tunnelStart);
		for(const BlockIndex& block : tunnel.getView(blocks))
			area.m_hasDigDesignations.designate(faction, block, nullptr);
		items.exit(pick);
		actors.equipment_add(dwarf1, pick);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		// Find project, reserve and activate.
		simulation.doStep();
		CHECK(items.reservable_isFullyReserved(pick, faction));
		CHECK(actors.project_get(dwarf1)->reservationsComplete());
		// Path to location.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, tunnelStart);
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		std::function<bool()> predicate = [&]() { return !blocks.solid_is(tunnelStart); };
		simulation.fastForwardUntillPredicate(predicate, 45);
		CHECK(!actors.objective_getCurrent<DigObjective>(dwarf1).getProject());
		CHECK(!blocks.isReserved(tunnelStart, faction));
		// Find next project, make reservations, and activate.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		Project* project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		CHECK(project);
		CHECK(project->getLocation() == tunnelSecond);
		CHECK(!actors.move_getDestination(dwarf1).exists());
		CHECK(!blocks.isReserved(tunnelStart, faction));
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		CHECK(project);
		CHECK(project->reservationsComplete());
		CHECK(items.reservable_hasAnyReservations(pick));
		CHECK(actors.move_hasPathRequest(dwarf1));
		// Path to location.
		simulation.doStep();
		CHECK(!actors.move_getPath(dwarf1).empty());
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		CHECK(project);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, tunnelSecond);
		project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		CHECK(project);
		CHECK(project->getLocation() == tunnelSecond);
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(tunnelSecond); };
		simulation.fastForwardUntillPredicate(predicate2, 45);
		actors.satisfyNeeds(dwarf1);
		// Find project.
		CHECK(actors.move_hasPathRequest(dwarf1));
		simulation.doStep();
		// Path to work spot.
		CHECK(actors.move_hasPathRequest(dwarf1));
		simulation.doStep();
		CHECK(actors.move_getPath(dwarf1).size() == 1);
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
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		actors.objective_setPriority(dwarf2, digObjectiveType.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf2) == L"dig");
		CHECK(actors.move_hasPathRequest(dwarf1));
		CHECK(actors.move_hasPathRequest(dwarf2));
		simulation.doStep();
		DigProject* project = actors.objective_getCurrent<DigObjective>(dwarf1).getProject();
		CHECK(project != nullptr);
		CHECK(actors.objective_getCurrent<DigObjective>(dwarf2).getProject() == project);
		CHECK(project->hasTryToHaulThreadedTask());
		// Both path to the pick, dwarf1 reserves it and dwarf2
		simulation.doStep();
		CHECK(actors.move_hasPathRequest(dwarf1));
		CHECK(actors.move_hasPathRequest(dwarf2));
		simulation.doStep();
		if(actors.move_destinationIsAdjacentToLocation(dwarf1, holeLocation))
			CHECK(actors.move_destinationIsAdjacentToLocation(dwarf2, pickLocation));
		else
		{
			CHECK(actors.move_destinationIsAdjacentToLocation(dwarf1, pickLocation));
			CHECK(actors.move_destinationIsAdjacentToLocation(dwarf2, holeLocation));
		}
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
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		actors.objective_setPriority(dwarf2, digObjectiveType.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf2) == L"dig");
		// Workers find closer project first.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.getActionDescription(dwarf1) == L"dig");
		DigProject& project1 = static_cast<DigProject&>(*actors.project_get(dwarf1));
		CHECK(project1.getLocation() == holeLocation1);
		CHECK(actors.project_get(dwarf1) == actors.project_get(dwarf2));
		// Reserve project.
		simulation.doStep();
		CHECK(project1.reservationsComplete());
		// Wait for first project to complete.
		std::function<bool()> predicate1 = [&]() { return !blocks.solid_is(holeLocation1); };
		simulation.fastForwardUntillPredicate(predicate1, 22);
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!actors.project_exists(dwarf2));
		CHECK(!items.reservable_hasAnyReservations(pick));
		// Workers find second project.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.getActionDescription(dwarf1) == L"dig");
		DigProject& project2 = static_cast<DigProject&>(*actors.project_get(dwarf1));
		CHECK(project2.getLocation() == holeLocation2);
		CHECK(actors.project_get(dwarf1) == actors.project_get(dwarf2));
		// Reserve second Project.
		simulation.doStep();
		CHECK(project2.reservationsComplete());
		// Wait for second project to complete.
		std::function<bool()> predicate2 = [&]() { return !blocks.solid_is(holeLocation2); };
		simulation.fastForwardUntillPredicate(predicate2, 22);
		simulation.doStep();
		CHECK(actors.getActionDescription(dwarf1) != L"dig");
		CHECK(actors.getActionDescription(dwarf2) != L"dig");
	}
	SUBCASE("cannot path to spot")
	{
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 3, 4), blocks.getIndex_i(8, 3, 4), dirt);
		BlockIndex gateway = blocks.getIndex_i(9, 3, 4);
		BlockIndex holeLocation = blocks.getIndex_i(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.getForFactionAndBlock(faction, holeLocation);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		CHECK(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		blocks.solid_set(gateway, dirt, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != L"dig");
		CHECK(!items.reservable_isFullyReserved(pick, faction));
		CHECK(project.getWorkers().empty());
	}
	SUBCASE("spot already dug")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		CHECK(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting holeLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		blocks.solid_setNot(holeLocation);
		CHECK(!actors.canPickUp_isCarryingItem(dwarf1, pick));
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!items.reservable_isFullyReserved(pick, faction));
		CHECK(!area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("pick destroyed")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 4, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDigDesignations.getForFactionAndBlock(faction, holeLocation);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		CHECK(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Destroying the pick triggers a dishonor callback which resets the project.
		items.destroy(pick);
		CHECK(project.getWorkers().empty());
		CHECK(!project.reservationsComplete());
		CHECK(!project.hasCandidate(dwarf1));
	}
	SUBCASE("player cancels")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
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
		CHECK(!actors.canPickUp_isCarryingItem(dwarf1, pick));
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!items.reservable_isFullyReserved(pick, faction));
		CHECK(!area.m_hasDigDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("player interrupts")
	{
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		BlockIndex goToLocation = blocks.getIndex_i(1, 8, 4);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		// One step to find the designation.
		simulation.doStep();
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		CHECK(actors.project_get(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		// Usurp the current objective, delaying the project.
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(goToLocation);
		actors.objective_addTaskToStart(dwarf1, std::move(objective));
		CHECK(actors.objective_getCurrentName(dwarf1) != L"dig");
		CHECK(!actors.canPickUp_isCarryingItem(dwarf1, pick));
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!items.reservable_isFullyReserved(pick, faction));
		CHECK(project.getWorkersAndCandidates().empty());
		CHECK(area.m_hasDigDesignations.areThereAnyForFaction(faction));
		simulation.fastForwardUntillActorIsAt(area, dwarf1, goToLocation);
		CHECK(actors.objective_getCurrentName(dwarf1) == L"dig");
		// One step to find the designation.
		simulation.doStep();
		CHECK(actors.project_get(dwarf1) == &project);
		// One step to activate the project and reserve the pick.
		simulation.doStep();
		CHECK(project.reservationsComplete());
		CHECK(items.reservable_isFullyReserved(pick, faction));
		// One step to select haul type.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject);
		// Another step to path to the pick.
		simulation.doStep();
		CHECK(items.isAdjacentToLocation(pick, actors.move_getDestination(dwarf1)));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, items.getLocation(pick));
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, pick));
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
		ActorReference dwarf2Ref = actors.m_referenceData.getReference(dwarf2);
		BlockIndex holeLocation = blocks.getIndex_i(8, 8, 3);
		BlockIndex goToLocation = blocks.getIndex_i(1, 8, 4);
		area.m_hasDigDesignations.designate(faction, holeLocation, nullptr);
		actors.objective_setPriority(dwarf1, digObjectiveType.getId(), Priority::create(100));
		actors.objective_setPriority(dwarf2, digObjectiveType.getId(), Priority::create(100));
		// One step to find the designation,  activate the project, and reserve the pick.
		simulation.doStep();
		// One step to path to the project.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		Project& project = *actors.project_get(dwarf1);
		CHECK(actors.project_get(dwarf2) == &project);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, holeLocation);
		CHECK(project.finishEventExists());
		CHECK(project.reservationsComplete());
		CHECK(items.reservable_hasAnyReservations(pick));
		SUBCASE("reset")
		{
			// Usurp the current objective, reseting the project.
			std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(goToLocation);
			actors.objective_addTaskToStart(dwarf1, std::move(objective));
			CHECK(!project.reservationsComplete());
			CHECK(!items.reservable_hasAnyReservations(pick));
			CHECK(actors.project_get(dwarf2) == &project);
			// Project is unable to reserve.
			simulation.doStep();
			CHECK(!project.reservationsComplete());
			CHECK(!actors.project_exists(dwarf2));
			CHECK(!actors.objective_getCurrent<DigObjective>(dwarf2).getJoinableProjectAt(area, holeLocation, dwarf2));
			// dwarf2 is unable to find another dig project, turns on objective type delay in objective priority set.
			simulation.doStep();
			CHECK(actors.objective_getCurrentName(dwarf2) == L"wander");
			CHECK(actors.move_hasPathRequest(dwarf2));
			CHECK(actors.objective_isOnDelay(dwarf2, ObjectiveType::getIdByName(L"dig")));
			simulation.fastForwardUntillActorIsAt(area, dwarf1, goToLocation);
			CHECK(!actors.project_exists(dwarf2));
			// One step to find the designation,  activate the project, and reserve the pick.
			simulation.doStep();
			CHECK(actors.project_exists(dwarf1));
			CHECK(project.reservationsComplete());
			// One step to path to the project.
			simulation.doStep();
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, holeLocation);
			CHECK(actors.objective_isOnDelay(dwarf2, ObjectiveType::getIdByName(L"dig")));
			Step dwarf2DelayEndsAt = actors.objective_getDelayEndFor(dwarf2, ObjectiveType::getIdByName(L"dig"));
			Step delay = dwarf2DelayEndsAt - simulation.m_step;
			simulation.fastForward(delay);
			CHECK(!actors.objective_isOnDelay(dwarf2, ObjectiveType::getIdByName(L"dig")));
			if(actors.move_hasPathRequest(dwarf2))
			{
				simulation.doStep();
				CHECK(actors.move_getDestination(dwarf2).exists());
			}
			else
			{
				CHECK(!actors.move_getPath(dwarf2).empty());
			}
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf2, actors.move_getDestination(dwarf2));
			simulation.fastForwardUntillPredicate([&](){ return actors.objective_getCurrentName(dwarf2) == L"dig"; });
			CHECK(actors.move_hasPathRequest(dwarf2));
			// Dwarf2 finds and joins project.
			simulation.doStep();
			CHECK(actors.project_exists(dwarf2));
			CHECK(actors.project_get(dwarf2) == &project);
			CHECK(actors.move_hasPathRequest(dwarf2));
			// Dwarf2 paths to project.
			simulation.doStep();
			CHECK(blocks.isAdjacentToIncludingCornersAndEdges(project.getLocation(), actors.move_getDestination(dwarf2)));
			CHECK(project.finishEventExists());
			CHECK(project.getPercentComplete() == 0);
		}
		SUBCASE("no reset")
		{
			std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(goToLocation);
			actors.objective_addTaskToStart(dwarf2, std::move(objective));
			CHECK(!actors.project_exists(dwarf2));
			CHECK(!project.getWorkers().contains(dwarf2Ref));
			CHECK(project.reservationsComplete());
			CHECK(actors.project_get(dwarf1) == &project);
		}
		auto holeDug = [&]{ return !blocks.solid_is(holeLocation); };
		// 23 minutes rather then 22 because dwarf2 is 'late'.
		simulation.fastForwardUntillPredicate(holeDug, 23);
		CHECK(!digObjectiveType.canBeAssigned(area, dwarf1));
		CHECK(!digObjectiveType.canBeAssigned(area, dwarf2));
	}
}
