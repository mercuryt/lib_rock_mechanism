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
		// Activete project and reserve all required.
		simulation.doStep();
		REQUIRE(dwarf1.m_project == &project);
		// Select a haul strategy and create a subproject.
		simulation.doStep();
		// Find a path.
		REQUIRE(dwarf1.m_canMove.getDestination() == nullptr);
		std::function<bool()> predicate = [&]() { return wallLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(!constructObjectiveType.canBeAssigned(dwarf1));
		REQUIRE(!wallLocation.m_hasDesignations.contains(faction, BlockDesignation::Construct));
	}
	SUBCASE("make two walls")
	{
		Block& wallLocation1 = area.m_blocks[8][4][2];
		Block& wallLocation2 = area.m_blocks[8][5][2];
		Item& boards = simulation.createItem(ItemType::byName("board"), wood, 50u);
		boards.setLocation(area.m_blocks[8][7][2]);
		Item& pegs = simulation.createItem(ItemType::byName("peg"), wood, 50u);
		pegs.setLocation(area.m_blocks[3][8][2]);
		Item& saw = simulation.createItem(ItemType::byName("saw"), MaterialType::byName("bronze"), 25u, 0u);
		saw.setLocation(area.m_blocks[5][7][2]);
		Item& mallet = simulation.createItem(ItemType::byName("mallet"), wood, 25u, 0u);
		mallet.setLocation(area.m_blocks[9][5][2]);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, nullptr, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		// Find project to join.
		std::function<bool()> predicate = [&]() { return wallLocation1.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate);
		predicate = [&]() { return wallLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
	}
	SUBCASE("make wall with two workers")
	{
		Actor& dwarf2 = simulation.createActor(dwarf, area.m_blocks[1][4][2]);
		dwarf2.setFaction(&faction);
		area.m_hasActors.add(dwarf2);
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
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "construct");
		// Find project to join.
		std::function<bool()> predicate = [&]() { return wallLocation.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "construct");
	}
	SUBCASE("make two walls with two workers")
	{
		Actor& dwarf2 = simulation.createActor(dwarf, area.m_blocks[1][4][2]);
		dwarf2.setFaction(&faction);
		area.m_hasActors.add(dwarf2);
		Block& wallLocation1 = area.m_blocks[8][4][2];
		Block& wallLocation2 = area.m_blocks[8][5][2];
		Item& boards = simulation.createItem(ItemType::byName("board"), wood, 50u);
		boards.setLocation(area.m_blocks[8][7][2]);
		Item& pegs = simulation.createItem(ItemType::byName("peg"), wood, 50u);
		pegs.setLocation(area.m_blocks[3][8][2]);
		Item& saw = simulation.createItem(ItemType::byName("saw"), MaterialType::byName("bronze"), 25u, 0u);
		saw.setLocation(area.m_blocks[5][7][2]);
		Item& mallet = simulation.createItem(ItemType::byName("mallet"), wood, 25u, 0u);
		mallet.setLocation(area.m_blocks[9][5][2]);
		area.m_hasConstructionDesignations.designate(faction, wallLocation1, nullptr, wood);
		area.m_hasConstructionDesignations.designate(faction, wallLocation2, nullptr, wood);
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		dwarf2.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() == "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() == "construct");
		// Find project to join.
		[[maybe_unused]] ConstructProject& project1 = area.m_hasConstructionDesignations.getProject(faction, wallLocation1);
		[[maybe_unused]] ConstructProject& project2 = area.m_hasConstructionDesignations.getProject(faction, wallLocation2);
		std::function<bool()> predicate = [&]() { return wallLocation1.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate);
		predicate = [&]() { return wallLocation2.isSolid(); };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(dwarf1.m_hasObjectives.getCurrent().name() != "construct");
		REQUIRE(dwarf2.m_hasObjectives.getCurrent().name() != "construct");
	}
	SUBCASE("make stairs")
	{
		Block& stairsLocation = area.m_blocks[8][4][2];
		Item& boards = simulation.createItem(ItemType::byName("board"), wood, 50u);
		boards.setLocation(area.m_blocks[8][7][2]);
		Item& pegs = simulation.createItem(ItemType::byName("peg"), wood, 50u);
		pegs.setLocation(area.m_blocks[3][8][2]);
		Item& saw = simulation.createItem(ItemType::byName("saw"), MaterialType::byName("bronze"), 25u, 0u);
		saw.setLocation(area.m_blocks[5][7][2]);
		Item& mallet = simulation.createItem(ItemType::byName("mallet"), wood, 25u, 0u);
		mallet.setLocation(area.m_blocks[9][5][2]);
		area.m_hasConstructionDesignations.designate(faction, stairsLocation, &BlockFeatureType::stairs, wood);
		REQUIRE(stairsLocation.m_hasDesignations.contains(faction, BlockDesignation::Construct));
		REQUIRE(area.m_hasConstructionDesignations.contains(faction, stairsLocation));
		REQUIRE(constructObjectiveType.canBeAssigned(dwarf1));
		dwarf1.m_hasObjectives.m_prioritySet.setPriority(constructObjectiveType, 100);
		std::function<bool()> predicate = [&]() { return stairsLocation.m_hasBlockFeatures.contains(BlockFeatureType::stairs); };
		simulation.fastForwardUntillPredicate(predicate);
		REQUIRE(stairsLocation.m_hasBlockFeatures.contains(BlockFeatureType::stairs));
	}
}
