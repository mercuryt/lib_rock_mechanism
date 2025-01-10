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
#include "objective.h"
#include <new>
TEST_CASE("woodcutting")
{
	ItemTypeId branch = ItemType::byName(L"branch");
	ItemTypeId log = ItemType::byName(L"log");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Plants& plants = area.getPlants();
	Items& items = area.getItems();
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName(L"dirt"));
	BlockIndex treeLocation = blocks.getIndex_i(5, 5, 1);
	PlantIndex tree = plants.create({.location=treeLocation, .species=PlantSpecies::byName(L"poplar tree")});
	REQUIRE(plants.getBlocks(tree).size() == 5);
	FactionId faction = simulation.createFaction(L"Tower of Power");
	area.m_blockDesignations.registerFaction(faction);
	area.m_hasWoodCuttingDesignations.addFaction(faction);
	ActorIndex dwarf = actors.create({
		.species=AnimalSpecies::byName(L"dwarf"),
		.percentGrown=Percent::create(100),
		.location=blocks.getIndex_i(1,1,1),
		.hasSidearm=false,
	});
	ActorReference dwarfRef = actors.m_referenceData.getReference(dwarf);
	actors.setFaction(dwarf, faction);
	ItemIndex axe = items.create({.itemType=ItemType::byName(L"axe"), .materialType=MaterialType::byName(L"bronze"), .quality=Quality::create(25u), .percentWear=Percent::create(10u)});
	area.m_hasWoodCuttingDesignations.designate(faction, treeLocation);
	const WoodCuttingObjectiveType& objectiveType = static_cast<const WoodCuttingObjectiveType&>(ObjectiveType::getByName(L"woodcutting"));
	REQUIRE(objectiveType.canBeAssigned(area, dwarf));
	actors.objective_setPriority(dwarf, objectiveType.getId(), Priority::create(10));
	Project* project = nullptr;
	SUBCASE("axe on ground")
	{
		items.setLocationAndFacing(axe, blocks.getIndex_i(3,8,1), Facing::create(0));
		// One step to find the designation, activate the project, and reserve the axe.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf));
		project = actors.project_get(dwarf);
		// One step to select haul type.
		simulation.doStep();
		REQUIRE(project->getProjectWorkerFor(dwarfRef).haulSubproject);
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
		REQUIRE(actors.objective_getCurrentName(dwarf) != L"woodcutting");
		REQUIRE(!actors.canPickUp_isCarryingItem(dwarf, axe));
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
	}
	SUBCASE("axe equiped, collect in stockpile")
	{
		BlockIndex branchStockpileLocation = blocks.getIndex_i(8,8,1);
		area.m_hasStockPiles.registerFaction(faction);
		area.m_hasStocks.addFaction(faction);
		StockPile& stockpile = area.m_hasStockPiles.getForFaction(faction).addStockPile({ItemQuery::create(branch)});
		stockpile.addBlock(branchStockpileLocation);
		const StockPileObjectiveType& stockPileObjectiveType = static_cast<const StockPileObjectiveType&>(ObjectiveType::getByName(L"stockpile"));
		actors.objective_setPriority(dwarf, stockPileObjectiveType.getId(), Priority::create(100));
		actors.equipment_add(dwarf, axe);
		// One step to find the designation, activate the project.
		simulation.doStep();
		REQUIRE(actors.project_exists(dwarf));
		project = actors.project_get(dwarf);
		// One step to path to the project
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, treeLocation);
		Step stepsDuration = project->getDuration();
		simulation.fastForward(stepsDuration);
		REQUIRE(!blocks.plant_exists(treeLocation));
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
		REQUIRE(!objectiveType.canBeAssigned(area, dwarf));
		REQUIRE(actors.objective_getCurrentName(dwarf) != L"woodcutting");
		REQUIRE(!actors.project_exists(dwarf));
		REQUIRE(actors.getActionDescription(dwarf) == L"stockpile");
		// One step to create a stockpile project.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf)->reservationsComplete());
		// Select haul strategy.
		simulation.doStep();
		REQUIRE(actors.project_get(dwarf)->getProjectWorkerFor(dwarfRef).haulSubproject);
		// Pickup and path.
		simulation.doStep();
		REQUIRE(actors.move_getDestination(dwarf).exists());
		REQUIRE(actors.canPickUp_exists(dwarf));
		simulation.fastForwardUntillActorIsAdjacentToDestination(area, dwarf, branchStockpileLocation);
		REQUIRE(!actors.canPickUp_exists(dwarf));
	}
}
