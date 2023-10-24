#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/dig.h"
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
	Actor& dwarf1 = simulation.createActor(dwarf, area.m_blocks[1][1][4]);
	Faction faction(L"tower of power");
	dwarf1.setFaction(&faction);
	Item& pick = simulation.createItem(ItemType::byName("pick"), bronze, 50u, 0u);
	pick.setLocation(area.m_blocks[5][5][4]);
	area.m_hasDiggingDesignations.addFaction(faction);
	SUBCASE("dig hole")
	{
		Block& holeLocation = area.m_blocks[8][4][3];
		area.m_hasDiggingDesignations.designate(faction, holeLocation, nullptr);
		DigProject& project = area.m_hasDiggingDesignations.at(faction, holeLocation);
		REQUIRE(holeLocation.m_hasDesignations.contains(faction, BlockDesignation::Dig));
		REQUIRE(area.m_hasDiggingDesignations.contains(faction, holeLocation));
		DigObjectiveType digObjectiveType;
		REQUIRE(digObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		// One step to select haul type.
		simulation.doStep();
		// Another step to path to the pick.
		simulation.doStep();
		// And another for some reason??
		simulation.doStep();
		REQUIRE(!dwarf1.m_canMove.getPath().empty());
		REQUIRE(pick.isAdjacentTo(*dwarf1.m_canMove.getDestination()));
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, *pick.m_location);
		REQUIRE(dwarf1.m_canPickup.isCarrying(pick));
		simulation.doStep();
		simulation.fastForwardUntillActorIsAdjacentToDestination(dwarf1, holeLocation);
		Step stepsDelay = project.getDelay();
		simulation.fastForward(stepsDelay);
		REQUIRE(!holeLocation.isSolid());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
	}
}
