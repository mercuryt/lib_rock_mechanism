#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/space/space.h"
#include "../../engine/items/items.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/projects/construct.h"
#include "../../engine/geometry/cuboid.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/config.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/numericTypes/types.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/objectives/construct.h"
#include "../../engine/items/items.h"
#include "../../engine/actors/actors.h"
#include "../../engine/space/space.h"
#include "../../engine/plants.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/portables.hpp"
#include "reference.h"
TEST_CASE("construct")
{
	static MaterialTypeId wood = MaterialType::byName("poplar wood");
	static MaterialTypeId marble = MaterialType::byName("marble");
	static AnimalSpeciesId dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
	FactionId faction = simulation.createFaction("Tower Of Power");
	area.m_spaceDesignations.registerFaction(faction);
	const ConstructObjectiveType& constructObjectiveType = static_cast<const ConstructObjectiveType&>(ObjectiveType::getByName("construct"));
	Point3D dwarf1Start = Point3D::create(1, 1, 2);
	ActorIndex dwarf1 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=dwarf1Start,
		.faction=faction,
	});
	ActorReference dwarf1Ref = actors.m_referenceData.getReference(dwarf1);
	area.m_hasConstructionDesignations.addFaction(faction);
	ItemIndex boards = items.create({
		.itemType=ItemType::byName("board"),
		.materialType=wood,
		.location=Point3D::create(8,7,2),
		.quantity=Quantity::create(50u),
	});
	ItemIndex pegs = items.create({
		.itemType=ItemType::byName("peg"),
		.materialType=wood,
		.location=Point3D::create(3, 8, 2),
		.quantity=Quantity::create(50u)
	});
	ItemIndex saw = items.create({
		.itemType=ItemType::byName("saw"),
		.materialType=MaterialType::byName("bronze"),
		.location=Point3D::create(5,7,2),
		.quality=Quality::create(25u),
		.percentWear=Percent::create(0)
	});
	ItemIndex mallet = items.create({
		.itemType=ItemType::byName("mallet"),
		.materialType=wood,
		.location=Point3D::create(9, 5, 2),
		.quality=Quality::create(25u),
		.percentWear=Percent::create(0),
	});
	SUBCASE("make wall")
	{
		Point3D wallLocation = Point3D::create(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		CHECK(space.designation_has(wallLocation, faction, SpaceDesignation::Construct));
		CHECK(area.m_hasConstructionDesignations.contains(faction, wallLocation));
		CHECK(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));
		CHECK(actors.objective_getCurrentName(dwarf1) == "construct");
		ConstructObjective& objective = actors.objective_getCurrent<ConstructObjective>(dwarf1);
		CHECK(objective.joinableProjectExistsAt(area, wallLocation, dwarf1));
		const auto& terrainFacade = area.m_hasTerrainFacades.getForMoveType(actors.getMoveType(dwarf1));
		CHECK(terrainFacade.accessable(dwarf1Start, actors.getFacing(dwarf1), wallLocation, dwarf1));
		// Search for accessable project, activete project and reserve all required.
		simulation.doStep();
		CHECK(actors.project_get(dwarf1) == &project);
		CHECK(project.reservationsComplete());
		CHECK(project.hasTryToHaulThreadedTask());
		// Four Item types are required but the mallet is already adjacent to the location.
		CHECK(project.getToPickup().size() == 3);
		// Select a haul strategy and create a subproject.
		simulation.doStep();
		const ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		// Find a path to the first item.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		CHECK(actors.canPickUp_exists(dwarf1));
		// Path to the project location.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		CHECK(project.getToPickup().size() == 2);
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(project.hasTryToHaulThreadedTask());
		CHECK(projectWorker.haulSubproject == nullptr);
		// Create haulSubproject.
		simulation.doStep();
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(actors.move_hasPathRequest(dwarf1));
		// Path to the second item.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == "construct");
		CHECK(!actors.move_hasPathRequest(dwarf1));
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		CHECK(actors.canPickUp_exists(dwarf1));
		// Path to the project location.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		CHECK(project.getToPickup().size() == 1);
		CHECK(!actors.canPickUp_exists(dwarf1));
		// Create haulSubproject.
		simulation.doStep();
		CHECK(projectWorker.haulSubproject != nullptr);
		CHECK(actors.move_hasPathRequest(dwarf1));
		// Path to the third item.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) == "construct");
		CHECK(!actors.move_hasPathRequest(dwarf1));
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		CHECK(actors.canPickUp_exists(dwarf1));
		// Path to the project location.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1).exists());
		simulation.fastForwardUntillActorIsAtDestination(area, dwarf1, actors.move_getDestination(dwarf1));
		CHECK(project.getToPickup().empty());
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(project.deliveriesComplete());
		// Deliveries complete, start construction.
		CHECK(project.finishEventExists());
		simulation.fastForwardUntillPredicate([&]()->bool { return space.solid_is(wallLocation); }, 180);
		CHECK(actors.objective_getCurrentName(dwarf1) != "construct");
		CHECK(!area.m_hasConstructionDesignations.contains(faction, wallLocation));
		CHECK(!constructObjectiveType.canBeAssigned(area, dwarf1));
		CHECK(!space.designation_has(wallLocation, faction, SpaceDesignation::Construct));
		CHECK(items.reservable_getUnreservedCount(boards, faction) == items.getQuantity(boards));
		CHECK(items.reservable_getUnreservedCount(pegs, faction) == items.getQuantity(pegs));
		CHECK(!items.reservable_isFullyReserved(saw, faction));
		CHECK(!items.reservable_isFullyReserved(mallet, faction));
		CHECK(!space.isReserved(wallLocation, faction));
	}
	SUBCASE("make two walls")
	{
		Point3D wallLocation1 = Point3D::create(8, 4, 2);
		Point3D wallLocation2 = Point3D::create(8, 5, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, PointFeatureTypeId::Null, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, PointFeatureTypeId::Null, wood);
		CHECK(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		CHECK(actors.objective_getCurrentName(dwarf1) == "construct");
		ConstructProject& project1 = area.m_hasConstructionDesignations.getProject(faction, wallLocation1);
		ConstructProject& project2 = area.m_hasConstructionDesignations.getProject(faction, wallLocation2);
		// Find project to join, activate and reserve.
		simulation.doStep();
		CHECK(project1.reservationsComplete());
		simulation.fastForwardUntillPredicate([&]()->bool { return project1.deliveriesComplete(); });
		simulation.fastForwardUntillPredicate([&]()->bool { return space.solid_is(wallLocation1); }, 180);
		simulation.doStep();
		CHECK(project2.reservationsComplete());
		simulation.fastForwardUntillPredicate([&]()->bool { return project2.deliveriesComplete(); });
		simulation.fastForwardUntillPredicate([&]()->bool { return space.solid_is(wallLocation2); }, 180);
		CHECK(actors.objective_getCurrentName(dwarf1) != "construct");
	}
	SUBCASE("make wall with two workers")
	{
		ActorIndex dwarf2 = actors.create(ActorParamaters{
			.species=dwarf,
			.location=Point3D::create(1, 4, 2),
			.faction=faction
		});
		Point3D wallLocation = Point3D::create(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, wood);
		CHECK(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		actors.objective_setPriority(dwarf2, constructObjectiveType.getId(), Priority::create(100));;
		CHECK(actors.objective_getCurrentName(dwarf1) == "construct");
		CHECK(actors.objective_getCurrentName(dwarf2) == "construct");
		// Find project to join.
		std::function<bool()> predicate = [&]() { return space.solid_is(wallLocation); };
		simulation.fastForwardUntillPredicate(predicate, 90);
		CHECK(actors.objective_getCurrentName(dwarf1) != "construct");
		CHECK(actors.objective_getCurrentName(dwarf2) != "construct");
	}
	SUBCASE("make two walls with two workers")
	{
		ActorIndex dwarf2 = actors.create({
			.species=dwarf,
			.location=Point3D::create(1, 8, 2),
			.faction=faction,
		});
		ActorReference dwarf2Ref = actors.m_referenceData.getReference(dwarf2);
		Point3D wallLocation1 = Point3D::create(2, 4, 2);
		Point3D wallLocation2 = Point3D::create(3, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, PointFeatureTypeId::Null, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, PointFeatureTypeId::Null, wood);
		CHECK(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		actors.objective_setPriority(dwarf2, constructObjectiveType.getId(), Priority::create(100));;
		CHECK(actors.objective_getCurrentName(dwarf1) == "construct");
		CHECK(actors.objective_getCurrentName(dwarf2) == "construct");
		ConstructProject* project1 = &area.m_hasConstructionDesignations.getProject(faction, wallLocation1);
		ConstructProject* project2 = &area.m_hasConstructionDesignations.getProject(faction, wallLocation2);
		CHECK(actors.objective_getCurrent<ConstructObjective>(dwarf2).getProjectWhichActorCanJoinAt(area, wallLocation2, dwarf2));
		CHECK(actors.move_hasPathRequest(dwarf2));
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
		CHECK(actors.objective_getCurrent<ConstructObjective>(dwarf2).getProjectWhichActorCanJoinAt(area, wallLocation2, dwarf2));
		CHECK(actors.project_get(dwarf1) == project1);
		CHECK(actors.project_get(dwarf2) == project2);
		CHECK(project1->reservationsComplete());
		CHECK(!project2->reservationsComplete());
		CHECK(project1->getWorkers().size() == 1);
		CHECK(project2->getWorkers().empty());
		CHECK(!project1->hasCandidate(dwarf1));
		CHECK(project2->hasCandidate(dwarf2));
		// Select a haul strategy and create a subproject for dwarf1, dwarf2 tries again to activate project2, this time failing to find required unreserved items and activating prohibition on the project at the objective instance.
		simulation.doStep();
		CHECK(!actors.objective_getCurrent<ConstructObjective>(dwarf2).getProjectWhichActorCanJoinAt(area, wallLocation2, dwarf2));
		const ProjectWorker& projectWorker1 = project1->getProjectWorkerFor(dwarf1Ref);
		CHECK(projectWorker1.haulSubproject != nullptr);
		CHECK(project1->getWorkers().size() == 1);
		CHECK(!project2->hasCandidate(dwarf2));
		CHECK(!actors.project_exists(dwarf2));
		// Find a path for dwarf1, dwarf2 seeks project to join, and finds project1.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1) .exists());
		CHECK(actors.project_get(dwarf1) == project1);
		CHECK(area.m_hasConstructionDesignations.contains(faction, wallLocation1));
		CHECK(area.m_hasConstructionDesignations.contains(faction, wallLocation2));
		CHECK(project1->getWorkers().contains(dwarf2Ref));
		// Build first wall segment.
		simulation.fastForwardUntillPredicate([&]()->bool { return space.solid_is(wallLocation1); }, 90);
		CHECK(!area.m_hasConstructionDesignations.contains(faction, wallLocation1));
		CHECK(area.m_hasConstructionDesignations.contains(faction, wallLocation2));
		CHECK(!project2->isOnDelay());
		CHECK(space.designation_has(project2->getLocation(), faction, SpaceDesignation::Construct));
		// Both dwarves seek a project to join and find project2. This does not require a step if they are already adjacent but does if they are not.
		CHECK(actors.objective_getCurrentName(dwarf1) == "construct");
		CHECK(actors.objective_getCurrentName(dwarf2) == "construct");
		simulation.doStep();
		// Project2 completes reservations and both dwarfs graduate from candidate to worker.
		simulation.doStep();
		CHECK(project2->getWorkers().contains(dwarf1Ref));
		CHECK(project2->getWorkers().contains(dwarf2Ref));
		CHECK(!project2->hasCandidate(dwarf1));
		CHECK(!project2->hasCandidate(dwarf2));
		// Try to set haul strategy, both dwarves try to generate a haul subproject.
		simulation.doStep();
		simulation.fastForwardUntillPredicate([&]()->bool { return space.solid_is(wallLocation2); }, 90);
		CHECK(actors.objective_getCurrentName(dwarf1) != "construct");
		CHECK(actors.objective_getCurrentName(dwarf2) != "construct");
		CHECK(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("make stairs")
	{
		Point3D stairsLocation = Point3D::create(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, stairsLocation, PointFeatureTypeId::Stairs, wood);
		CHECK(space.designation_has(stairsLocation, faction, SpaceDesignation::Construct));
		CHECK(area.m_hasConstructionDesignations.contains(faction, stairsLocation));
		CHECK(constructObjectiveType.canBeAssigned(area, dwarf1));
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, stairsLocation);
		// Search for project and find stairs and reserve all required items.
		simulation.doStep();
		CHECK(project.reservationsComplete());
		// Select a haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject != nullptr);
		// Find a path.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1) .exists());
		std::function<bool()> predicate = [&]() { return space.pointFeature_contains(stairsLocation, PointFeatureTypeId::Stairs); };
		simulation.fastForwardUntillPredicate(predicate, 180);
		CHECK(space.pointFeature_contains(stairsLocation, PointFeatureTypeId::Stairs));
	}
	SUBCASE("cannot path to spot")
	{
		areaBuilderUtil::setSolidWall(area, Point3D::create(0, 3, 2), Point3D::create(8, 3, 2), wood);
		Point3D gateway = Point3D::create(9, 3, 2);
		Point3D wallLocation = Point3D::create(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project, and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		space.solid_set(gateway, wood, false);
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, gateway);
		// Cannot detour or find alternative block.
		simulation.doStep();
		CHECK(actors.objective_getCurrentName(dwarf1) != "construct");
		CHECK(!items.reservable_isFullyReserved(saw, faction));
		CHECK(project.getWorkers().empty());
	}
	SUBCASE("spot has become solid")
	{
		Point3D wallLocation = Point3D::create(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, wood);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		space.solid_set(wallLocation, wood, false);
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!items.reservable_isFullyReserved(saw, faction));
		CHECK(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("saw destroyed")
	{
		Point3D wallLocation = Point3D::create(8, 4, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, wood);
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
		CHECK(project.getWorkers().empty());
		CHECK(!project.reservationsComplete());
		CHECK(!project.hasCandidate(dwarf1));
	}
	SUBCASE("player cancels")
	{
		Point3D wallLocation = Point3D::create(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, wood);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the pick.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		area.m_hasConstructionDesignations.undesignate(faction, wallLocation);
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!items.reservable_isFullyReserved(saw, faction));
		CHECK(!area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
	SUBCASE("player interrupts")
	{
		Point3D wallLocation = Point3D::create(8, 8, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, wood);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the tools and materials.
		simulation.doStep();
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// Setting wallLocation as not solid immideatly triggers the project locationDishonorCallback, which cancels the project.
		std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(Point3D::create(0, 8, 2));
		actors.objective_addTaskToStart(dwarf1, std::move(objective));
		CHECK(actors.objective_getCurrentName(dwarf1) != "construct");
		CHECK(!actors.project_exists(dwarf1));
		CHECK(!items.reservable_isFullyReserved(saw, faction));
		CHECK(area.m_hasConstructionDesignations.areThereAnyForFaction(faction));
	}
}
TEST_CASE("constructDirtWall")
{
	static const MaterialTypeId& dirt = MaterialType::byName("dirt");
	static const ItemTypeId& pile = ItemType::byName("pile");
	static const AnimalSpeciesId& dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	FactionId faction = simulation.createFaction("Tower Of Power");
	area.m_spaceDesignations.registerFaction(faction);
	const ConstructObjectiveType& constructObjectiveType = static_cast<const ConstructObjectiveType&>(ObjectiveType::getByName("construct"));
	Point3D dwarf1Start = Point3D::create(1, 1, 2);
	ActorIndex dwarf1 = actors.create(ActorParamaters{
		.species=dwarf,
		.location=dwarf1Start,
		.faction=faction,
	});
	ActorReference dwarf1Ref = actors.m_referenceData.getReference(dwarf1);
	area.m_hasConstructionDesignations.addFaction(faction);
	SUBCASE("dirt wall")
	{
		ItemIndex dirtPile = items.create({.itemType=pile, .materialType=dirt, .location=Point3D::create(9, 9, 2), .quantity=Quantity::create(150u)});
		Point3D wallLocation = Point3D::create(3, 3, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		Quantity dirtPerLoad = actors.canPickUp_maximumNumberWhichCanBeCarriedWithMinimumSpeed(dwarf1, items.getSingleUnitMass(dirtPile), Config::minimumHaulSpeedInital);
		// One step to find the designation, activate the project and reserve the pile.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.getActionDescription(dwarf1) == "construct");
		ConstructProject& project = static_cast<ConstructProject&>(*actors.project_get(dwarf1));
		CHECK(items.reservable_hasAnyReservations(dirtPile));
		CHECK(project.reservationsComplete());
		CHECK(project.getWorkers().contains(dwarf1Ref));
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pile.
		simulation.doStep();
		auto& consumed = MaterialType::construction_getConsumed(dirt);
		auto found = std::ranges::find_if(consumed, [dirtPile, &area](auto pair) { return pair.first.query(area, dirtPile); });
		CHECK(found != consumed.end());
		Quantity quantity = found->second;
		CHECK(quantity <= items.getQuantity(dirtPile));
		Quantity trips = Quantity::create(std::ceil((float)quantity.get() / (float)dirtPerLoad.get()));
		for(uint i = 0; i < trips; ++i)
		{
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, items.getLocation(dirtPile));
			if(!actors.canPickUp_isCarryingItem(dwarf1, dirtPile))
				CHECK(actors.canPickUp_isCarryingItemGeneric(dwarf1, pile, dirt, dirtPerLoad));
			simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
			CHECK(!actors.canPickUp_exists(dwarf1));
			CHECK(actors.project_exists(dwarf1));
			if(!project.deliveriesComplete())
			{
				CHECK(project.hasTryToHaulThreadedTask());
				simulation.doStep();
				CHECK(actors.move_hasPathRequest(dwarf1));
			}
		}
		CHECK(actors.project_get(dwarf1) == &project);
		CHECK(actors.getActionDescription(dwarf1) == "construct");
		CHECK(project.deliveriesComplete());
		CHECK(project.getFinishStep() != 0);
		simulation.fastForwardUntillPredicate([&]{ return space.solid_is(wallLocation); }, 130);
		CHECK(area.getTotalCountOfItemTypeOnSurface(pile) == 0);
	}
	SUBCASE("dirt wall three piles")
	{
		Point3D pileLocation1 = Point3D::create(9, 9, 2);
		Point3D pileLocation2 = Point3D::create(9, 7, 2);
		Point3D pileLocation3 = Point3D::create(9, 5, 2);
		ItemIndex dirtPile1 = items.create({.itemType=pile, .materialType=dirt, .location=pileLocation1, .quantity=Quantity::create(50u)});
		ItemIndex dirtPile2 = items.create({.itemType=pile, .materialType=dirt, .location=pileLocation2, .quantity=Quantity::create(50u)});
		ItemIndex dirtPile3 = items.create({.itemType=pile, .materialType=dirt, .location=pileLocation3, .quantity=Quantity::create(50u)});
		Point3D wallLocation = Point3D::create(9, 0, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation,activate the project and reserve the pile.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.getActionDescription(dwarf1) == "construct");
		ConstructProject& project = static_cast<ConstructProject&>(*actors.project_get(dwarf1));
		CHECK(items.reservable_hasAnyReservations(dirtPile1));
		CHECK(items.reservable_hasAnyReservations(dirtPile2));
		CHECK(items.reservable_hasAnyReservations(dirtPile3));
		CHECK(project.reservationsComplete());
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to a pile.
		simulation.doStep();
		// Move pile 1.
		// Pile 1 is the closest to the worker's starting position.
		auto pile1Moved = [&]{ return space.item_getCount(pileLocation1, pile, dirt) == 0; };
		simulation.fastForwardUntillPredicate(pile1Moved);
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, dirtPile1));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
		CHECK(!actors.canPickUp_exists(dwarf1));
		// After Dropping off pile 1 the next closest is pile 3.
		// Move pile 3.
		auto pile3Moved = [&]{ return space.item_getCount(pileLocation3, pile, dirt) == 0; };
		simulation.fastForwardUntillPredicate(pile3Moved);
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, dirtPile3));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
		CHECK(!actors.canPickUp_exists(dwarf1));
		// The last Pile is pile 2.
		// Move pile 2.
		auto pile2Moved = [&]{ return space.item_getCount(pileLocation2, pile, dirt) == 0; };
		simulation.fastForwardUntillPredicate(pile2Moved);
		CHECK(actors.canPickUp_isCarryingItem(dwarf1, dirtPile2));
		simulation.fastForwardUntillActorIsAdjacentToLocation(area, dwarf1, wallLocation);
		CHECK(!actors.canPickUp_exists(dwarf1));
		CHECK(project.deliveriesComplete());
		CHECK(project.getFinishStep() != 0);
		auto predicate = [&]{ return space.solid_is(wallLocation); };
		simulation.fastForwardUntillPredicate(predicate, 130);
	}
	SUBCASE("dirt wall not enough dirt")
	{
		ItemTypeId pile = ItemType::byName("pile");
		MaterialTypeId dirt = MaterialType::byName("dirt");
		ItemIndex dirtPile = items.create({.itemType=pile, .materialType=dirt, .location=Point3D::create(9, 9, 2), .quantity=Quantity::create(140u)});
		Point3D wallLocation = Point3D::create(3, 3, 2);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		// One step to find the designation but fail to activate
		simulation.doStep();
		CHECK(!items.reservable_hasAnyReservations(dirtPile));
		CHECK(!project.reservationsComplete());
		CHECK(project.getWorkers().empty());
		CHECK(!project.hasCandidate(dwarf1));
		CHECK(!actors.project_exists(dwarf1));
		ConstructObjective& objective = actors.objective_getCurrent<ConstructObjective>(dwarf1);
		CHECK(!objective.getProject());
		CHECK(!objective.joinableProjectExistsAt(area, wallLocation, dwarf1));
		// One step to fail to find another construction project.
		simulation.doStep();
		CHECK(!actors.project_exists(dwarf1));
		CHECK(actors.getActionDescription(dwarf1) != "construct");
	}
	SUBCASE("dirt wall multiple small piles")
	{
		ItemTypeId pile = ItemType::byName("pile");
		MaterialTypeId dirt = MaterialType::byName("dirt");
		Point3D wallLocation = Point3D::create(3, 3, 2);
		Cuboid pileLocation(Point3D::create(9, 9, 2), Point3D::create(0, 9, 2));
		for(const Point3D& block : pileLocation)
			items.create({.itemType=pile, .materialType=dirt, .location=block, .quantity=Quantity::create(15)});
		area.m_hasConstructionDesignations.designate(faction, wallLocation, PointFeatureTypeId::Null, dirt);
		actors.objective_setPriority(dwarf1, constructObjectiveType.getId(), Priority::create(100));;
		// One step to find the designation, activate the project and reserve the piles.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf1));
		CHECK(actors.getActionDescription(dwarf1) == "construct");
		ConstructProject& project = static_cast<ConstructProject&>(*actors.project_get(dwarf1));
		ItemIndex firstInBlock = space.item_getAll(Point3D::create(8,9,2)).front();
		CHECK(items.reservable_hasAnyReservations(firstInBlock));
		CHECK(project.reservationsComplete());
		// One step to pick a haul strategy.
		simulation.doStep();
		CHECK(project.getProjectWorkerFor(dwarf1Ref).haulSubproject);
		// One step to path.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf1) .exists());
		auto predicate = [&]{ return space.solid_is(wallLocation); };
		simulation.fastForwardUntillPredicate(predicate, 130);
	}
}
