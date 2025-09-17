#include "../../lib/doctest.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/items/items.h"
#include "../../engine/definitions/itemType.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/definitions/plantSpecies.h"
#include "../../engine/objectives/woodcutting.h"
#include "../../engine/objectives/stockpile.h"
#include "objective.h"
#include <new>
TEST_CASE("woodcutting")
{
	ItemTypeId branch = ItemType::byName("branch");
	ItemTypeId log = ItemType::byName("log");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("dirt"));
	Point3D treeLocation = Point3D::create(5, 5, 1);
	PlantIndex tree = plants.create({.location=treeLocation, .species=PlantSpecies::byName("poplar tree")});
	CHECK(plants.getOccupied(tree).volume() == 5);
	FactionId faction = simulation.createFaction("Tower of Power");
	area.m_spaceDesignations.registerFaction(faction);
	area.m_hasWoodCuttingDesignations.addFaction(faction);
	ActorIndex dwarf = actors.create({
		.species=AnimalSpecies::byName("dwarf"),
		.percentGrown=Percent::create(100),
		.location=Point3D::create(1,1,1),
		.hasSidearm=false,
	});
	ActorReference dwarfRef = actors.m_referenceData.getReference(dwarf);
	actors.setFaction(dwarf, faction);
	ItemIndex axe = items.create({.itemType=ItemType::byName("axe"), .materialType=MaterialType::byName("bronze"), .quality=Quality::create(25u), .percentWear=Percent::create(10u)});
	area.m_hasWoodCuttingDesignations.designate(faction, treeLocation);
	const WoodCuttingObjectiveType& objectiveType = static_cast<const WoodCuttingObjectiveType&>(ObjectiveType::getByName("woodcutting"));
	CHECK(objectiveType.canBeAssigned(area, dwarf));
	actors.objective_setPriority(dwarf, objectiveType.getId(), Priority::create(10));
	Project* project = nullptr;
	SUBCASE("axe on ground")
	{
		items.location_set(axe, Point3D::create(3,8,1), Facing4::North);
		// One step to find the designation, activate the project, and reserve the axe.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf));
		project = actors.project_get(dwarf);
		// One step to select haul type.
		simulation.doStep();
		CHECK(project->getProjectWorkerFor(dwarfRef).haulSubproject);
		// Another step to path to the axe.
		simulation.doStep();
		CHECK(!actors.move_getPath(dwarf).empty());
		CHECK(items.isAdjacentToLocation(axe, actors.move_getDestination(dwarf)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, items.getLocation(axe));
		CHECK(actors.canPickUp_isCarryingItem(dwarf, axe));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, treeLocation);
		Step stepsDuration = project->getDuration();
		simulation.fastForward(stepsDuration);
		CHECK(!space.plant_exists(treeLocation));
		CHECK(actors.objective_getCurrentName(dwarf) != "woodcutting");
		CHECK(!actors.canPickUp_isCarryingItem(dwarf, axe));
		CHECK(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		CHECK(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
	}
	SUBCASE("axe equiped, collect in stockpile")
	{
		Point3D branchStockpileLocation = Point3D::create(8,8,1);
		area.m_hasStockPiles.registerFaction(faction);
		area.m_hasStocks.addFaction(faction);
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile({ItemQuery::create(branch)});
		stockpile.addPoint(branchStockpileLocation);
		const StockPileObjectiveType& stockPileObjectiveType = static_cast<const StockPileObjectiveType&>(ObjectiveType::getByName("stockpile"));
		actors.objective_setPriority(dwarf, stockPileObjectiveType.getId(), Priority::create(100));
		actors.equipment_add(dwarf, axe);
		// One step to find the designation, activate the project.
		simulation.doStep();
		CHECK(actors.project_exists(dwarf));
		project = actors.project_get(dwarf);
		// One step to path to the project
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, treeLocation);
		Step stepsDuration = project->getDuration();
		simulation.fastForward(stepsDuration);
		CHECK(!space.plant_exists(treeLocation));
		CHECK(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		CHECK(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
		CHECK(!objectiveType.canBeAssigned(area, dwarf));
		CHECK(actors.objective_getCurrentName(dwarf) != "woodcutting");
		CHECK(!actors.project_exists(dwarf));
		CHECK(actors.getActionDescription(dwarf) == "stockpile");
		// One step to create a stockpile project.
		simulation.doStep();
		CHECK(actors.project_get(dwarf)->reservationsComplete());
		// Select haul strategy.
		simulation.doStep();
		CHECK(actors.project_get(dwarf)->getProjectWorkerFor(dwarfRef).haulSubproject);
		// Pickup and path.
		simulation.doStep();
		CHECK(actors.move_getDestination(dwarf).exists());
		CHECK(actors.canPickUp_exists(dwarf));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, branchStockpileLocation);
		CHECK(!actors.canPickUp_exists(dwarf));
	}
}
