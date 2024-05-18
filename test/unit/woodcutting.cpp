#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasItems.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
#include <new>
TEST_CASE("woodcutting")
{
	const ItemType& branch = ItemType::byName("branch");
	const ItemType& log = ItemType::byName("log");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("dirt"));
	Block& treeLocation = area.getBlock(5, 5, 1);
	treeLocation.m_hasPlant.createPlant(PlantSpecies::byName("poplar tree"), 100);
	REQUIRE(treeLocation.m_hasPlant.get().m_blocks.size() == 5);
	static Faction faction(L"Tower of Power");
	area.m_blockDesignations.registerFaction(faction);
	area.m_hasWoodCuttingDesignations.addFaction(faction);
	Actor& dwarf = simulation.m_hasActors->createActor(ActorParamaters{
		.species=AnimalSpecies::byName("dwarf"), 
		.percentGrown=100,
		.location=&area.getBlock(1,1,1), 
		.hasSidearm=false,
	});
	dwarf.setFaction(&faction);
	Item& axe = simulation.m_hasItems->createItemNongeneric(ItemType::byName("axe"), MaterialType::byName("bronze"), 25u, 10u);
	area.m_hasWoodCuttingDesignations.designate(faction, treeLocation);
	const WoodCuttingObjectiveType objectiveType;
	REQUIRE(objectiveType.canBeAssigned(dwarf));
	dwarf.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
	Project* project = nullptr;
	SUBCASE("axe on ground")
	{
		axe.setLocation(area.getBlock(3,8,1));
		// One step to find the designation.
		simulation.doStep();
		REQUIRE(dwarf.m_project);
		project = dwarf.m_project;
		REQUIRE(project->hasCandidate(dwarf));
		// One step to activate the project and reserve the axe.
		simulation.doStep();
		REQUIRE(!project->hasCandidate(dwarf));
		REQUIRE(dwarf.m_project == project);
		// One step to select haul type.
		simulation.doStep();
		REQUIRE(project->getProjectWorkerFor(dwarf).haulSubproject);
		// Another step to path to the axe.
		simulation.doStep();
		REQUIRE(!dwarf.m_canMove.getPath().empty());
		REQUIRE(axe.isAdjacentTo(*dwarf.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf, *axe.m_location);
		REQUIRE(dwarf.m_canPickup.isCarrying(axe));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf, treeLocation);
		Step stepsDuration = project->getDuration();
		simulation.fastForward(stepsDuration);
		REQUIRE(!treeLocation.m_hasPlant.exists());
		REQUIRE(dwarf.m_hasObjectives.getCurrent().getObjectiveTypeId() != ObjectiveTypeId::WoodCutting);
		REQUIRE(!dwarf.m_canPickup.isCarrying(axe));
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
	}
	SUBCASE("axe equiped, collect in stockpile")
	{
		Block& branchStockpileLocation = area.getBlock(8,8,1);
		area.m_hasStockPiles.registerFaction(faction);
		area.m_hasStocks.addFaction(faction);
		StockPile& stockpile = area.m_hasStockPiles.at(faction).addStockPile({{branch, nullptr}});
		stockpile.addBlock(branchStockpileLocation);
		StockPileObjectiveType stockPileObjectiveType;
		dwarf.m_hasObjectives.m_prioritySet.setPriority(stockPileObjectiveType, 10);
		dwarf.m_equipmentSet.addEquipment(axe);
		// One step to find the designation.
		simulation.doStep();
		REQUIRE(dwarf.m_project);
		project = dwarf.m_project;
		REQUIRE(project->hasCandidate(dwarf));
		// One step to activate the project.
		simulation.doStep();
		REQUIRE(!project->hasCandidate(dwarf));
		REQUIRE(dwarf.m_project == project);
		// One step to path to the project
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf, treeLocation);
		Step stepsDuration = project->getDuration();
		// TODO: why plus 2?
		simulation.fastForward(stepsDuration + 2);
		REQUIRE(!treeLocation.m_hasPlant.exists());
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(log) == 10);
		REQUIRE(area.getTotalCountOfItemTypeOnSurface(branch) == 20);
		REQUIRE(!objectiveType.canBeAssigned(dwarf));
		REQUIRE(dwarf.m_hasObjectives.getCurrent().getObjectiveTypeId() != ObjectiveTypeId::WoodCutting);
		REQUIRE(!dwarf.m_project);
		REQUIRE(dwarf.getActionDescription() == L"stockpile");
		// One step to create a stockpile project.
		simulation.doStep();
		REQUIRE(dwarf.m_project);
		// Reserve.
		simulation.doStep();
		REQUIRE(dwarf.m_project->reservationsComplete());
		// Select haul strategy.
		simulation.doStep();
		REQUIRE(dwarf.m_project->getProjectWorkerFor(dwarf).haulSubproject);
		// Pickup and path.
		simulation.doStep();
		REQUIRE(dwarf.m_canMove.getDestination());
		REQUIRE(dwarf.m_canPickup.exists());
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf, branchStockpileLocation);
		REQUIRE(!dwarf.m_canPickup.exists());
	}
}
