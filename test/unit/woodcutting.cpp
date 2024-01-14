#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
#include <new>
TEST_CASE("woodcutting")
{
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, MaterialType::byName("dirt"));
	Block& treeLocation = area.getBlock(5, 5, 1);
	treeLocation.m_hasPlant.createPlant(PlantSpecies::byName("poplar tree"), 100);
	assert(treeLocation.m_hasPlant.get().m_blocks.size() == 5);
	static const Faction faction(L"Tower of Power");
	area.m_hasWoodCuttingDesignations.addFaction(faction);
	Actor& dwarf = simulation.createActor(AnimalSpecies::byName("dwarf"), area.getBlock(1,1,1), 100);
	dwarf.setFaction(&faction);
	Item& axe = simulation.createItemNongeneric(ItemType::byName("axe"), MaterialType::byName("bronze"), 25u, 10u);
	axe.setLocation(area.getBlock(3,8,1));
	area.m_hasWoodCuttingDesignations.designate(faction, treeLocation);
	const WoodCuttingObjectiveType objectiveType;
	REQUIRE(objectiveType.canBeAssigned(dwarf));
	dwarf.m_hasObjectives.m_prioritySet.setPriority(objectiveType, 10);
	// One step to find the designation.
	simulation.doStep();
	assert(dwarf.m_project);
	Project& project = *dwarf.m_project;
	assert(project.hasCandidate(dwarf));
	// One step to activate the project and reserve the axe.
	simulation.doStep();
	assert(!project.hasCandidate(dwarf));
	assert(dwarf.m_project == &project);
	// One step to select haul type.
	simulation.doStep();
	assert(project.getProjectWorkerFor(dwarf).haulSubproject);
	// Another step to path to the axe.
	simulation.doStep();
	REQUIRE(!dwarf.m_canMove.getPath().empty());
	REQUIRE(axe.isAdjacentTo(*dwarf.m_canMove.getDestination()));
	simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf, *axe.m_location);
	REQUIRE(dwarf.m_canPickup.isCarrying(axe));
	simulation.doStep();
	simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf, treeLocation);
	Step stepsDuration = project.getDuration();
	simulation.fastForward(stepsDuration);
	REQUIRE(!treeLocation.m_hasPlant.exists());
	REQUIRE(dwarf.m_hasObjectives.getCurrent().getObjectiveTypeId() != ObjectiveTypeId::WoodCutting);
	REQUIRE(!dwarf.m_canPickup.isCarrying(axe));
	REQUIRE(area.getTotalCountOfItemTypeOnSurface(ItemType::byName("log")) == 10);
	REQUIRE(area.getTotalCountOfItemTypeOnSurface(ItemType::byName("branch")) == 20);
}
