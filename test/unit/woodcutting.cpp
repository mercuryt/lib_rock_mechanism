#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actors/actors.h"
#include "../../engine/plants.h"
#include "../../engine/items/items.h"
#include "../../engine/itemType.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/objectives/woodcutting.h"
#include "../../engine/objectives/stockpile.h"
#include <new>
TEST_CASE("woodcutting")
{
	const ItemType& branch = ItemType::byName("branch");
	const ItemType& log = ItemType::byName("log");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("dirt"));
	BlockIndex treeLocation = blocks.getIndex_i(5, 5, 1);
	PlantIndex tree = plants.create({.location=treeLocation, .species=PlantSpecies::byName("poplar tree")});
	REQUIRE(plants.getBlocks(tree).size() == 5);
	FactionId faction = simulation.createFaction(L"Tower Of Power").id;
	area.m_blockDesignations.registerFaction(faction);
	area.m_hasWoodCuttingDesignations.addFaction(faction);
	ActorIndex dwarf = actors.create({
		.species=AnimalSpecies::byName("dwarf"), 
		.percentGrown=Percent::create(100),
		.location=blocks.getIndex_i(1,1,1), 
		.hasSidearm=false,
	});
	actors.setFaction(dwarf, faction);
	ItemIndex axe = items.create({.itemType=ItemType::byName("axe"), .materialType=MaterialType::byName("bronze"), .quality=Quality::create(25u), .percentWear=Percent::create(10u)});
	area.m_hasWoodCuttingDesignations.designate(faction, treeLocation);
	const WoodCuttingObjectiveType objectiveType;
	REQUIRE(objectiveType.canBeAssigned(area, dwarf));
	actors.objective_setPriority(dwarf, objectiveType, Priority::create(10));
	Project* project = nullptr;
	SUBCASE("axe on ground")
	{
		items.setLocation(axe, blocks.getIndex_i(3,8,1));
		// One step to find the designation.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf));
		project = actors.project_get(dwarf);
		REQUIRE(project->hasCandidate(dwarf));
		// One step to activate the project and reserve the axe.
		simulation.doStep();
		REQUIRE(!project->hasCandidate(dwarf));
		REQUIRE(actors.project_get(dwarf) == project);
		// One step to select haul type.
		simulation.doStep();
		REQUIRE(project->getProjectWorkerFor(dwarf.toReference(area)).haulSubproject);
		// Another step to path to the axe.
		simulation.doStep();
		REQUIRE(!actors.move_getPath(dwarf).empty());
		REQUIRE(items.isAdjacentToLocation(axe, actors.move_getDestination(dwarf)));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, items.getLocation(axe));
		REQUIRE(actors.canPickUp_isCarryingItem(dwarf, axe));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, treeLocation);
		Step stepsDuration = project->getDuration();
		simulation.fastForward(stepsDuration);
		REQUIRE(!blocks.plant_exists(treeLocation));
		REQUIRE(actors.objective_getCurrent<Objective>(dwarf).getObjectiveTypeId() != ObjectiveTypeId::WoodCutting);
		REQUIRE(!actors.canPickUp_isCarryingItem(dwarf, axe));
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
	}
	SUBCASE("axe equiped, collect in stockpile")
	{
		BlockIndex branchStockpileLocation = blocks.getIndex_i(8,8,1);
		area.m_hasStockPiles.registerFaction(faction);
		area.m_hasStocks.addFaction(faction);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile({{branch, nullptr}});
		stockpile.addBlock(branchStockpileLocation);
		StockPileObjectiveType stockPileObjectiveType;
		actors.objective_setPriority(dwarf, stockPileObjectiveType, Priority::create(100));
		actors.equipment_add(dwarf, axe);
		// One step to find the designation.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf));
		project = actors.project_get(dwarf);
		REQUIRE(project->hasCandidate(dwarf));
		// One step to activate the project.
		simulation.doStep();
		REQUIRE(!project->hasCandidate(dwarf));
		REQUIRE(actors.project_get(dwarf) == project);
		// One step to path to the project
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, treeLocation);
		Step stepsDuration = project->getDuration();
		// TODO: why plus 2?
		simulation.fastForward(stepsDuration + 2);
		REQUIRE(!blocks.plant_exists(treeLocation));
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
		REQUIRE(!objectiveType.canBeAssigned(area, dwarf));
		REQUIRE(actors.objective_getCurrent<Objective>(dwarf).getObjectiveTypeId() != ObjectiveTypeId::WoodCutting);
		REQUIRE(!actors.project_exists(dwarf));
		REQUIRE(actors.getActionDescription(dwarf) == L"stockpile");
		// One step to create a stockpile project.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf));
		// Reserve.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf)->reservationsComplete());
		// Select haul strategy.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf)->getProjectWorkerFor(dwarf.toReference(area)).haulSubproject);
		// Pickup and path.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf).exists());
		REQUIRE(actors.canPickUp_exists(dwarf));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, branchStockpileLocation);
		REQUIRE(!actors.canPickUp_exists(dwarf));
	}
}
