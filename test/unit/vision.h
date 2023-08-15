#include "../../lib/doctest.h"
#include "../../src/actor.h"
#include "../../src/area.h"
#include "../../src/block.h"
#include "../../src/areaBuilderUtil.h"
#include "../../src/animalSpecies.h"
#include "../../src/simulation.h"
TEST_CASE("vision")
{
	simulation::init();
	Area area(10,10,10,0);
	auto& marble = MaterialType::byName("marble");
	auto& glass = MaterialType::byName("glass");
	auto& door = BlockFeatureType::door;
	auto& hatch = BlockFeatureType::hatch;
	auto& stairs = BlockFeatureType::stairs;
	auto& floor = BlockFeatureType::floor;
	auto& dwarf = AnimalSpecies::byName("dwarf");
	auto& troll = AnimalSpecies::byName("troll");

	SUBCASE("See no one when no one is present to be seen")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block = area.m_blocks[5][5][1];
		Actor actor = Actor::create(dwarf, block);
		VisionRequest visionRequest(actor);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.empty());
	}
	SUBCASE("See someone nearby")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[3][3][1];
		Block& block2 = area.m_blocks[7][7][1];
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block2);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
		CHECK(visionRequest.m_actors.contains(&a2));
	}
	SUBCASE("Too far to see")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[0][0][1];
		Block& block2 = area.m_blocks[19][19][1];
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block2);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 0);
	}
	SUBCASE("Vision blocked by wall")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][5][1];
		Block& block3 = area.m_blocks[5][7][1];
		block2.setSolid(marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 0);
	}
	SUBCASE("Vision not blocked by wall not directly in the line of sight")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][5][1];
		Block& block3 = area.m_blocks[6][6][1];
		block2.setSolid(marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
	}
	SUBCASE("Vision not blocked by one by one wall for two by two shape")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][5][1];
		Block& block3 = area.m_blocks[5][7][1];
		block2.setSolid(marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(troll, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
	}
	SUBCASE("Vision not blocked by glass wall")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][5][1];
		Block& block3 = area.m_blocks[5][7][1];
		block2.setSolid(glass);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
	}
	SUBCASE("Vision blocked by closed door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][5][1];
		Block& block3 = area.m_blocks[5][7][1];
		block2.m_hasBlockFeatures.construct(door, marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 0);
		block2.m_hasBlockFeatures.at(door)->closed = false;
		VisionRequest visionRequest2(a1);
		visionRequest2.readStep();
		CHECK(visionRequest2.m_actors.size() == 1);
	}
	SUBCASE("Vision from above and below blocked by closed hatch")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		Block& block1 = area.m_blocks[5][5][1];
		Block& block2 = area.m_blocks[5][5][2];
		Block& block3 = area.m_blocks[5][5][3];
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		block3.m_hasBlockFeatures.construct(hatch, marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 0);
		VisionRequest visionRequest2(a1);
		visionRequest2.readStep();
		CHECK(visionRequest2.m_actors.size() == 0);
		block3.m_hasBlockFeatures.at(hatch)->closed = false;
		VisionRequest visionRequest3(a1);
		visionRequest3.readStep();
		CHECK(visionRequest3.m_actors.size() == 1);
	}
	SUBCASE("Vision not blocked by closed hatch on the same z level")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][5][1];
		Block& block3 = area.m_blocks[5][7][1];
		block2.m_hasBlockFeatures.construct(hatch, marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
	}
	SUBCASE("Vision from above and below blocked by floor")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		Block& block1 = area.m_blocks[5][5][1];
		Block& block2 = area.m_blocks[5][5][2];
		Block& block3 = area.m_blocks[5][5][3];
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		block2.m_hasBlockFeatures.construct(stairs, marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
		block3.m_hasBlockFeatures.construct(floor, marble);
		VisionRequest visionRequest2(a1);
		visionRequest2.readStep();
		CHECK(visionRequest2.m_actors.size() == 0);
		VisionRequest visionRequest3(a2);
		visionRequest3.readStep();
		CHECK(visionRequest3.m_actors.size() == 0);
	}
	SUBCASE("Vision from below not blocked by glass floor")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		Block& block1 = area.m_blocks[5][5][1];
		Block& block2 = area.m_blocks[5][5][2];
		Block& block3 = area.m_blocks[5][5][3];
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		block2.m_hasBlockFeatures.construct(stairs, marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
		block3.m_hasBlockFeatures.construct(floor, glass);
		VisionRequest visionRequest2(a1);
		visionRequest2.readStep();
		CHECK(visionRequest2.m_actors.size() == 1);
	}
	SUBCASE("Vision not blocked by floor on the same z level")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block1 = area.m_blocks[5][3][1];
		Block& block2 = area.m_blocks[5][5][1];
		Block& block3 = area.m_blocks[5][7][1];
		block2.m_hasBlockFeatures.construct(floor, marble);
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block3);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
	}
	SUBCASE("VisionCuboid setup")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.visionCuboidsActivate();
		CHECK(area.m_visionCuboids.size() == 1);
	}
	SUBCASE("VisionCuboid divide and join")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.visionCuboidsActivate();
		Block& block1 = area.m_blocks[1][1][1];
		Block& block2 = area.m_blocks[5][5][2];
		Block& block3 = area.m_blocks[5][5][5];
		Block& block4 = area.m_blocks[1][1][7];
		Block& block5 = area.m_blocks[9][9][1];
		CHECK(area.m_visionCuboids.size() == 1);
		CHECK(block1.m_visionCuboid->m_cuboid.size() == 900);
		CHECK(block1.m_visionCuboid == block2.m_visionCuboid);
		CHECK(block1.m_visionCuboid == block3.m_visionCuboid);
		CHECK(block1.m_visionCuboid == block4.m_visionCuboid);
		CHECK(block1.m_visionCuboid == block5.m_visionCuboid);
		block3.m_hasBlockFeatures.construct(floor, marble);
		VisionCuboid::clearDestroyed(area);
		CHECK(area.m_visionCuboids.size() == 2);
		CHECK(block1.m_visionCuboid->m_cuboid.size() == 400);
		CHECK(block4.m_visionCuboid->m_cuboid.size() == 500);
		CHECK(block1.m_visionCuboid == block2.m_visionCuboid);
		CHECK(block1.m_visionCuboid != block3.m_visionCuboid);
		CHECK(block1.m_visionCuboid != block4.m_visionCuboid);
		CHECK(block1.m_visionCuboid == block5.m_visionCuboid);
		block2.setSolid(marble);
		VisionCuboid::clearDestroyed(area);
		CHECK(area.m_visionCuboids.size() == 7);
		CHECK(block2.m_visionCuboid == nullptr);
		CHECK(block1.m_visionCuboid != block3.m_visionCuboid);
		CHECK(block1.m_visionCuboid != block4.m_visionCuboid);
		CHECK(block1.m_visionCuboid != block5.m_visionCuboid);
		block3.m_hasBlockFeatures.remove(floor);
		VisionCuboid::clearDestroyed(area);
		CHECK(area.m_visionCuboids.size() == 7);
		CHECK(block3.m_visionCuboid == block4.m_visionCuboid);
		CHECK(block4.m_visionCuboid->m_cuboid.size() == 500);
		block2.setNotSolid();
		VisionCuboid::clearDestroyed(area);
		CHECK(area.m_visionCuboids.size() == 1);
		CHECK(block2.m_visionCuboid != nullptr);
		CHECK(block1.m_visionCuboid == block2.m_visionCuboid);
		CHECK(block1.m_visionCuboid == block3.m_visionCuboid);
		CHECK(block1.m_visionCuboid == block4.m_visionCuboid);
		CHECK(block1.m_visionCuboid == block5.m_visionCuboid);
	}
	SUBCASE("VisionCuboid can see")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		area.visionCuboidsActivate();
		Block& block1 = area.m_blocks[3][3][1];
		Block& block2 = area.m_blocks[7][7][1];
		Actor a1 = Actor::create(dwarf, block1);
		Actor a2 = Actor::create(dwarf, block2);
		VisionRequest visionRequest(a1);
		visionRequest.readStep();
		CHECK(visionRequest.m_actors.size() == 1);
		CHECK(visionRequest.m_actors.contains(&a2));
	}
}
