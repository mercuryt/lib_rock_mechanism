#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/construct.h"
#include "../../src/cuboid.h"
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
	Actor& dwarf1 = simulation.createActor(dwarf, area.m_blocks[1][1][2]);
	dwarf1.setFaction(&faction);
	area.m_hasActors.add(dwarf1);
	area.m_hasConstructionDesignations.addFaction(faction);
	SUBCASE("make wall")
	{
		Block& wallLocation = area.m_blocks[8][4][2];
		Item& boards = simulation.createItem(ItemType::byName("board"), wood, 50u);
		boards.setLocation(area.m_blocks[8][7][2]);
		Item& pegs = simulation.createItem(ItemType::byName("peg"), wood, 50u);
		pegs.setLocation(area.m_blocks[3][8][2]);
		Item& saw = simulation.createItem(ItemType::byName("saw"), MaterialType::byName("bronze"), 25u, 0u);
		saw.setLocation(area.m_blocks[5][7][2]);
		Item& mallet = simulation.createItem(ItemType::byName("mallet"), wood, 25u, 0u);
		mallet.setLocation(area.m_blocks[9][5][2]);
		area.m_hasConstructionDesignations.designate(faction, wallLocation, nullptr, wood);
		ConstructProject& project = area.m_hasConstructionDesignations.getProject(faction, wallLocation);
		REQUIRE(wallLocation.m_hasDesignations.contains(faction, BlockDesignation::Construct));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, wallLocation));
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		// Find project to join.
		simulation.doStep();
		REQUIRE(dwarf1.m_project != nullptr); 
		REQUIRE(dwarf1.m_project == &project);
		REQUIRE(dwarf1.m_canMove.getDestination() == nullptr);
		std::function<bool()> predicate = [&]() { return wallLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(!constructObjectiveType.canBeAssigned(dwarf1));
		REQUIRE(!wallLocation.m_hasDesignations.contains(faction, BlockDesignation::Construct));
	}
}
