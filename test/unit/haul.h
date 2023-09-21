#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/simulation.h"
#include "../../src/project.h"
#include "../../src/haul.h"
#include "../../src/targetedHaul.h"
TEST_CASE("haul")
{
	static const MaterialType& dirt = MaterialType::byName("dirt");
	static const MaterialType& marble = MaterialType::byName("marble");
	static const MaterialType& gold = MaterialType::byName("gold");
	static const MaterialType& poplarWood = MaterialType::byName("poplar wood");
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const ItemType& chunk = ItemType::byName("chunk");
	static const ItemType& wheelBarrow = ItemType::byName("wheel barrow");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayers(area, 0, 1, dirt);
	Actor& dwarf1 = simulation.createActor(dwarf, area.m_blocks[1][1][2]);
	Faction faction(L"tower of power");
	dwarf1.setFaction(&faction);
	REQUIRE(!dwarf1.m_canPickup.exists());
	SUBCASE("canPickup")
	{
		Block& chunkLocation = area.m_blocks[1][2][2];
		Item& chunk1 = simulation.createItem(chunk, marble, 1);
		chunk1.setLocation(chunkLocation);
		REQUIRE(dwarf1.m_canPickup.canPickupAny(chunk1));
		dwarf1.m_canPickup.pickUp(chunk1, 1);
		Block& destination = area.m_blocks[5][5][2];
		dwarf1.m_canMove.setDestination(destination);
		simulation.doStep();
		REQUIRE(!dwarf1.m_canMove.getPath().empty());
		while(dwarf1.m_location != &destination)
			simulation.doStep();
		dwarf1.m_canPickup.putDown(*dwarf1.m_location);
		REQUIRE(chunk1.m_location == &destination);
	}
	SUBCASE("has haul tools")
	{
		Block& wheelBarrowLocation = area.m_blocks[1][2][2];
		Item& wheelBarrow1 = simulation.createItem(wheelBarrow, poplarWood, 3u, 0u);
		Item& chunk1 = simulation.createItem(chunk, gold, 1u);
		wheelBarrow1.setLocation(wheelBarrowLocation);
		area.m_hasHaulTools.registerHaulTool(wheelBarrow1);
		REQUIRE(area.m_hasHaulTools.hasToolToHaul(faction, chunk1));
	}
	SUBCASE("individual haul strategy")
	{
		Block& destination = area.m_blocks[5][5][2];
		Block& chunkLocation = area.m_blocks[1][5][2];
		Item& chunk1 = simulation.createItem(chunk, marble, 1u);
		chunk1.setLocation(chunkLocation);
		TargetedHaulProject& project = area.m_targetedHauling.begin(std::vector<Actor*>({&dwarf1}), chunk1, destination);
		// One step to run the create subproject threaded task.
		simulation.doStep();
		// Another step to find the path.
		simulation.doStep();
		ProjectWorker& projectWorker = project.getProjectWorkerFor(dwarf1);
		REQUIRE(projectWorker.haulSubproject != nullptr);
		REQUIRE(projectWorker.haulSubproject->getHaulStrategy() == HaulStrategy::Individual);
		REQUIRE(dwarf1.m_canMove.getPath().size() != 0);
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		Block& destinationIntermediate = *dwarf1.m_canMove.getDestination();
		REQUIRE(destinationIntermediate.isAdjacentToIncludingCornersAndEdges(chunkLocation));
		while(dwarf1.m_location != &destinationIntermediate)
			simulation.doStep();
		REQUIRE(dwarf1.m_canPickup.exists());
		REQUIRE(dwarf1.m_canPickup.getItem() == chunk1);
		simulation.doStep();
		REQUIRE(dwarf1.m_canMove.getDestination() != nullptr);
		Block& destinationIntermediate2 = *dwarf1.m_canMove.getDestination();
		REQUIRE(destinationIntermediate2.isAdjacentToIncludingCornersAndEdges(destination));
		while(dwarf1.m_location != &destinationIntermediate2)
			simulation.doStep();
		simulation.fastForward(Config::addToStockPileDelaySteps);
		REQUIRE(chunk1.m_location == &destination);
		REQUIRE(!dwarf1.m_canPickup.exists());
	}
	SUBCASE("hand cart haul strategy")
	{
	}
	SUBCASE("panniers haul strategy")
	{
	}
	SUBCASE("animal cart haul strategy")
	{
	}
	SUBCASE("team haul strategy")
	{
	}
	SUBCASE("team hand cart haul strategy")
	{
	}
}
