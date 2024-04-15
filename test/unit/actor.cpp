#include "../../lib/doctest.h"
#include "../../engine/simulation.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/actor.h"
#include "config.h"
#include "fluidType.h"
TEST_CASE("actor")
{
	Step step = DateTime(10, 60, 10000).toSteps();
	Simulation simulation(L"", step);
	static const AnimalSpecies& dwarf = AnimalSpecies::byName("dwarf");
	static const AnimalSpecies& troll = AnimalSpecies::byName("troll");
	static const MaterialType& marble = MaterialType::byName("marble");
	Area& area = simulation.createArea(10,10,10);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Block& origin1 = area.getBlock(5, 5, 1);
	Block& origin2 = area.getBlock(7, 7, 1);
	Block& block1 = area.getBlock(6, 7, 1);
	Block& block2 = area.getBlock(7, 8, 1);
	Block& block3 = area.getBlock(6, 8, 1);
	SUBCASE("single tile")
	{
		int previousEventCount = simulation.m_eventSchedule.count();
		Actor& dwarf1 = simulation.createActor(ActorParamaters{
			.species=dwarf, 
			.percentGrown=100,
			.location=&origin1
		});
		REQUIRE(dwarf1.m_name == L"dwarf1");
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 100);
		REQUIRE(!dwarf1.m_canGrow.isGrowing());
		REQUIRE(dwarf1.m_shape->name == "oneByOneFull");
		REQUIRE(simulation.m_eventSchedule.count() - previousEventCount == 3);
		REQUIRE(dwarf1.m_location == &origin1);
		REQUIRE(dwarf1.m_location->m_hasShapes.contains(dwarf1));
		REQUIRE(dwarf1.m_canFight.getCombatScore() != 0);
	}
	SUBCASE("multi tile")
	{
		// Multi tile.
		Actor& troll1 = simulation.createActor(ActorParamaters{
			.species=troll, 
			.percentGrown=100,
			.location=&origin2
		});
		REQUIRE(origin2.m_hasShapes.contains(troll1));
		REQUIRE(block1.m_hasShapes.contains(troll1));
		REQUIRE(block2.m_hasShapes.contains(troll1));
		REQUIRE(block3.m_hasShapes.contains(troll1));
	}
	SUBCASE("growth")
	{
		Actor& dwarf1 = simulation.createActor(ActorParamaters{
			.species=dwarf, 
			.percentGrown=45,
			.location=&origin1,
		});
		area.getBlock(7, 7, 0).setNotSolid();
		area.getBlock(7, 7, 0).addFluid(100, FluidType::byName("water"));
		Item& food = simulation.createItemGeneric(ItemType::byName("apple"), MaterialType::byName("fruit"), 1000);
		food.setLocation(block1);
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 0);
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 45);
		REQUIRE(dwarf1.m_canGrow.isGrowing());
		Step nextPercentIncreaseStep = dwarf1.m_canGrow.getEvent().getStep();
		REQUIRE(nextPercentIncreaseStep <= simulation.m_step + Config::stepsPerDay * 95);
		REQUIRE(nextPercentIncreaseStep >= simulation.m_step + Config::stepsPerDay * 90);
		simulation.fastForward(Config::stepsPerDay);
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 45);
		REQUIRE(!dwarf1.m_canGrow.isGrowing());
		simulation.fastForward(Config::stepsPerHour * 2);
		REQUIRE(dwarf1.m_canGrow.isGrowing());
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 1);
		REQUIRE(dwarf1.m_canGrow.isGrowing());
		//TODO: This is too slow for unit tests, move to integration and replace with a version which sets event percent complete explicitly.
		/*
		simulation.fastForward(Config::stepsPerDay * 5);
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 6);
		simulation.fastForward((Config::stepsPerDay * 50));
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 60);
		simulation.fastForward((Config::stepsPerDay * 36));
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 99);
		simulation.fastForwardUntillPredicate([&]{ return dwarf1.m_canGrow.growthPercent() != 45; }, 60 * 24 * 40);
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 46);
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 0);
		REQUIRE(dwarf1.m_canGrow.isGrowing());
		*/
		dwarf1.m_canGrow.setGrowthPercent(20);
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 20);
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 0);
		REQUIRE(dwarf1.m_canGrow.isGrowing());
		dwarf1.m_canGrow.setGrowthPercent(100);
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 100);
		REQUIRE(dwarf1.m_canGrow.getEvent().isPaused());
		REQUIRE(!dwarf1.m_canGrow.isGrowing());
		dwarf1.m_canGrow.setGrowthPercent(30);
		REQUIRE(dwarf1.m_canGrow.growthPercent() == 30);
		REQUIRE(dwarf1.m_canGrow.getEventPercentComplete() == 0);
		REQUIRE(dwarf1.m_canGrow.isGrowing());
	}
}
