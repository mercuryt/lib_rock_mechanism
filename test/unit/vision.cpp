#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/animalSpecies.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/config.h"
TEST_CASE("vision")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	auto marble = MaterialType::byName(L"marble");
	auto glass = MaterialType::byName(L"glass");
	auto& door = BlockFeatureType::door;
	auto& hatch = BlockFeatureType::hatch;
	auto& stairs = BlockFeatureType::stairs;
	auto& floor = BlockFeatureType::floor;
	auto dwarf = AnimalSpecies::byName(L"dwarf");
	auto troll = AnimalSpecies::byName(L"troll");
	SUBCASE("See no one when no one is present to be seen")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block = blocks.getIndex_i(5, 5, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=block,
		});
		area.m_visionRequests.doStep();
		REQUIRE(actors.vision_getCanSee(actor).empty());
	}
	SUBCASE("See someone nearby")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(3, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(7, 7, 1);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block2)),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block2,
		});
		const Point3D block1Coordinates = blocks.getCoordinates(block1);
		const Point3D block2Coordinates = blocks.getCoordinates(block2);
		REQUIRE(block2Coordinates.isInFrontOf(block1Coordinates, actors.getFacing(a1)));
		REQUIRE(area.m_opacityFacade.hasLineOfSight(block1, block2));
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
		REQUIRE(result.contains(actors.getReference(a2)));
	}
	SUBCASE("Vision blocked by wall")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 1);
		REQUIRE(area.m_opacityFacade.hasLineOfSight(block1, block3));
		blocks.solid_set(block2, marble, false);
		REQUIRE(!area.m_opacityFacade.hasLineOfSight(block1, block3));
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 0);
	}
	SUBCASE("Vision not blocked by wall not directly in the line of sight")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(6, 6, 1);
		blocks.solid_set(block2, marble, false);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
	}
	SUBCASE("Vision not blocked by one by one wall for two by two shape")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 2, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 1);
		BlockIndex block4 = blocks.getIndex_i(4, 7, 1);
		blocks.solid_set(block2, marble, false);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		ActorIndex a2 = actors.create({
			.species=troll,
			.location=block3,
		});
		REQUIRE(actors.getBlocks(a2).contains(block4));
		const Point3D block1Coordinates = blocks.getCoordinates(block1);
		const Point3D block3Coordinates = blocks.getCoordinates(block3);
		const Point3D block4Coordinates = blocks.getCoordinates(block4);
		REQUIRE(block3Coordinates.isInFrontOf(block1Coordinates, actors.getFacing(a1)));
		REQUIRE(block4Coordinates.isInFrontOf(block1Coordinates, actors.getFacing(a1)));
		REQUIRE(area.m_opacityFacade.hasLineOfSight(block1, block4));
		REQUIRE(!area.m_opacityFacade.hasLineOfSight(block1, block3));
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
	}
	SUBCASE("Vision not blocked by glass wall")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 1);
		blocks.solid_set(block2, glass, false);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
	}
	SUBCASE("Vision blocked by closed door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 1);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		blocks.blockFeature_construct(block2, door, marble);
		bool canSeeThrough = blocks.canSeeThrough(block2);
		REQUIRE(!canSeeThrough);
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 0);
		blocks.blockFeature_open(block2, door);
		REQUIRE(blocks.canSeeThroughFrom(block2, block1));
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a1);
		REQUIRE(result2.size() == 1);
	}
	SUBCASE("Vision from above and below blocked by closed hatch")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 3);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		REQUIRE(area.m_opacityFacade.hasLineOfSight(block1, block3));
		REQUIRE(blocks.getCoordinates(block3).isInFrontOf(blocks.getCoordinates(block1), Facing4::North));
		blocks.blockFeature_construct(block3, hatch, marble);
		REQUIRE(!area.m_opacityFacade.hasLineOfSight(block1, block3));
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 0);
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a2);
		REQUIRE(result2.size() == 0);
		blocks.blockFeature_open(block3, hatch);
		REQUIRE(area.m_opacityFacade.hasLineOfSight(block1, block3));
		REQUIRE(area.m_visionRequests.size() == 2);
		area.m_visionRequests.doStep();
		auto result3 = actors.vision_getCanSee(a1);
		REQUIRE(result3.size() == 1);
	}
	SUBCASE("Vision not blocked by closed hatch on the same z level")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 1);
		blocks.blockFeature_construct(block2, hatch, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
	}
	SUBCASE("Vision from above and below blocked by floor")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 3);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		blocks.blockFeature_construct(block2, stairs, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
		blocks.blockFeature_construct(block3, floor, marble);
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a1);
		REQUIRE(result2.size() == 0);
		area.m_visionRequests.doStep();
		auto result3 = actors.vision_getCanSee(a2);
		REQUIRE(result3.size() == 0);
	}
	SUBCASE("Vision from below not blocked by glass floor")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 3);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		blocks.blockFeature_construct(block2, stairs, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
		blocks.blockFeature_construct(block3, floor, glass);
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a1);
		REQUIRE(result2.size() == 1);
	}
	SUBCASE("Vision not blocked by floor on the same z level")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 1);
		blocks.blockFeature_construct(block2, floor, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block3)),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
	}
	SUBCASE("VisionCuboid setup")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		REQUIRE(area.m_visionCuboids.size() == 1);
	}
	SUBCASE("build ground after activating")
	{
		REQUIRE(area.m_visionCuboids.size() == 1);
		blocks.solid_set(blocks.getIndex_i(0,0,0), marble, false);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 3);
		blocks.solid_set(blocks.getIndex_i(1,0,0), marble, false);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 3);
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(2, 0, 0), blocks.getIndex_i(9, 0, 0), marble);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 2);
		areaBuilderUtil::setSolidWall(area, blocks.getIndex_i(0, 1, 0), blocks.getIndex_i(9, 9, 0), marble);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 1);
	}
	SUBCASE("VisionCuboid divide and join")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 5);
		BlockIndex block4 = blocks.getIndex_i(1, 1, 7);
		BlockIndex block5 = blocks.getIndex_i(9, 9, 1);
		REQUIRE(area.m_visionCuboids.size() == 1);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1)->m_cuboid.size() == 900);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block2));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block3));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block4));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block5));
		blocks.blockFeature_construct(block3, floor, marble);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 2);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1)->m_cuboid.size() == 400);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block4)->m_cuboid.size() == 500);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block2));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) != area.m_visionCuboids.maybeGetForBlock(block3));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) != area.m_visionCuboids.maybeGetForBlock(block4));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block5));
		blocks.solid_set(block2, marble, false);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 7);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block2) == nullptr);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) != area.m_visionCuboids.maybeGetForBlock(block3));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) != area.m_visionCuboids.maybeGetForBlock(block4));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block5));
		blocks.blockFeature_remove(block3, floor);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 7);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block3) == area.m_visionCuboids.maybeGetForBlock(block4));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block4)->m_cuboid.size() == 500);
		blocks.solid_setNot(block2);
		area.m_visionCuboids.clearDestroyed(area);
		REQUIRE(area.m_visionCuboids.size() == 1);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block2) != nullptr);
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block2));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block3));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block4));
		REQUIRE(area.m_visionCuboids.maybeGetForBlock(block1) == area.m_visionCuboids.maybeGetForBlock(block5));
	}
	SUBCASE("VisionCuboid can see")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block1 = blocks.getIndex_i(3, 3, 1);
		BlockIndex block2 = blocks.getIndex_i(7, 7, 1);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block2)),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block2,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		REQUIRE(result.size() == 1);
		REQUIRE(result.contains(actors.getReference(a2)));
	}
}
TEST_CASE("Too far to see")
{
	auto marble = MaterialType::byName(L"marble");
	auto dwarf = AnimalSpecies::byName(L"dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(20,20,20);
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	BlockIndex block1 = blocks.getIndex_i(0, 0, 1);
	BlockIndex block2 = blocks.getIndex_i(19, 19, 1);
	ActorIndex a1 = actors.create({
		.species=dwarf,
		.location=block1,
		.facing=blocks.getCoordinates(block1).getFacingTwords(blocks.getCoordinates(block2)),
	});
	actors.create({
		.species=dwarf,
		.location=block2,
	});
	area.m_visionRequests.doStep();
	auto result = actors.vision_getCanSee(a1);
	REQUIRE(result.size() == 0);
}
