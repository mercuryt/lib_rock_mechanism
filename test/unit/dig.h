#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/dig.h"
#include "../../src/cuboid.h"
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
	Actor& dwarf1 = simulation.createActor(dwarf, area.m_blocks[1][1][4]);
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
		REQUIRE(digObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		// One step to activate the project and reserve the pick.
		simulation.doStep();
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
		Step stepsDuration = project.getDuration();
		simulation.fastForward(stepsDuration);
		REQUIRE(!holeLocation.isSolid());
		REQUIRE(holeLocation.m_hasBlockFeatures.empty());
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "dig");
		REQUIRE(!dwarf1.m_canPickup.isCarrying(pick));
	}
	SUBCASE("dig stairs and tunnel")
	{
		Block& stairsLocation1 = area.m_blocks[8][4][3];
		Block& stairsLocation2 = area.m_blocks[8][4][2];
		Block& aboveStairs = area.m_blocks[8][4][4];
		Block& tunnelStart = area.m_blocks[8][5][2];
		Block& tunnelEnd = area.m_blocks[8][6][2];
		Cuboid tunnel(tunnelEnd, tunnelStart);
		area.m_hasDiggingDesignations.designate(faction, stairsLocation1, &BlockFeatureType::stairs);
		area.m_hasDiggingDesignations.designate(faction, stairsLocation2, &BlockFeatureType::stairs);
		for(Block& block : tunnel)
			area.m_hasDiggingDesignations.designate(faction, block, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !stairsLocation1.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, Step(51 * 1000));
		REQUIRE(stairsLocation1.m_hasBlockFeatures.contains(BlockFeatureType::stairs));
		REQUIRE(stairsLocation1.m_hasShapes.canEnterEverFrom(dwarf1, aboveStairs));
		std::function<bool()> predicate2 = [&]() { return !stairsLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate2, Step(51 * 1000));
		std::function<bool()> predicate3 = [&]() { return !tunnelEnd.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate3, Step(101 * 1000));
	}
	SUBCASE("two workers")
	{
		Actor& dwarf2 = simulation.createActor(dwarf, area.m_blocks[1][2][4]);
		dwarf2.setFaction(&faction);
		Block& holeLocation = area.m_blocks[8][4][3];
		area.m_hasDiggingDesignations.designate(faction, holeLocation, nullptr);
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "dig");
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(digObjectiveType, 100);
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "dig");
		std::function<bool()> predicate = [&]() { return !holeLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate, Step(26 * 1000));
	}
}
