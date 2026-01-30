#include "../../lib/doctest.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/definitions/animalSpecies.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/config/config.h"
TEST_CASE("vision")
{
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	auto marble = MaterialType::byName("marble");
	auto glass = MaterialType::byName("glass");
	auto door = PointFeatureTypeId::Door;
	auto hatch = PointFeatureTypeId::Hatch;
	auto stairs = PointFeatureTypeId::Stairs;
	auto floor = PointFeatureTypeId::Floor;
	auto dwarf = AnimalSpecies::byName("dwarf");
	auto troll = AnimalSpecies::byName("troll");
	SUBCASE("See no one when no one is present to be seen")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block = Point3D::create(5, 5, 1);
		ActorIndex actor = actors.create({
			.species=dwarf,
			.location=block,
		});
		area.m_visionRequests.doStep();
		CHECK(actors.vision_getCanSee(actor).empty());
	}
	SUBCASE("See someone nearby")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(3, 3, 1);
		Point3D block2 = Point3D::create(7, 7, 1);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block2),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block2,
		});
		const Point3D block1Coordinates = block1;
		const Point3D block2Coordinates = block2;
		CHECK(block2Coordinates.isInFrontOf(block1Coordinates, actors.getFacing(a1)));
		CHECK(area.m_opacityFacade.hasLineOfSight(block1, block2));
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
		CHECK(result.contains(actors.getReference(a2)));
	}
	SUBCASE("Vision blocked by wall")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block2 = Point3D::create(5, 5, 1);
		Point3D block3 = Point3D::create(5, 7, 1);
		CHECK(area.m_opacityFacade.hasLineOfSight(block1, block3));
		space.solid_set(block2, marble, false);
		CHECK(!area.m_opacityFacade.hasLineOfSight(block1, block3));
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 0);
	}
	SUBCASE("Vision not blocked by wall not directly in the line of sight")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block2 = Point3D::create(5, 5, 1);
		Point3D block3 = Point3D::create(6, 6, 1);
		space.solid_set(block2, marble, false);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
	}
	SUBCASE("Vision not blocked by one by one wall for two by two shape")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 2, 1);
		Point3D block2 = Point3D::create(5, 5, 1);
		Point3D block3 = Point3D::create(5, 7, 1);
		Point3D block4 = Point3D::create(6, 7, 1);
		space.solid_set(block2, marble, false);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		ActorIndex a2 = actors.create({
			.species=troll,
			.location=block3,
			.facing=Facing4::South,
		});
		CHECK(actors.getOccupied(a2).contains(block4));
		const Point3D block1Coordinates = block1;
		const Point3D block3Coordinates = block3;
		const Point3D block4Coordinates = block4;
		CHECK(block3Coordinates.isInFrontOf(block1Coordinates, actors.getFacing(a1)));
		CHECK(block4Coordinates.isInFrontOf(block1Coordinates, actors.getFacing(a1)));
		CHECK(area.m_opacityFacade.hasLineOfSight(block1, block4));
		CHECK(!area.m_opacityFacade.hasLineOfSight(block1, block3));
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
	}
	SUBCASE("Vision not blocked by glass wall")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block2 = Point3D::create(5, 5, 1);
		Point3D block3 = Point3D::create(5, 7, 1);
		space.solid_set(block2, glass, false);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
	}
	SUBCASE("Vision blocked by closed door")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block2 = Point3D::create(5, 5, 1);
		Point3D block3 = Point3D::create(5, 7, 1);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		space.pointFeature_construct(block2, door, marble);
		bool canSeeThrough = space.canSeeThrough(block2);
		CHECK(!canSeeThrough);
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 0);
		space.pointFeature_open(block2, door);
		CHECK(space.canSeeThroughFrom(block2, block1));
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a1);
		CHECK(result2.size() == 1);
	}
	SUBCASE("Vision from above and below blocked by closed hatch")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		Point3D block1 = Point3D::create(5, 5, 1);
		Point3D block2 = Point3D::create(5, 5, 2);
		Point3D block3 = Point3D::create(5, 5, 3);
		space.solid_setNot(block1);
		space.solid_setNot(block2);
		space.solid_setNot(block3);
		CHECK(area.m_opacityFacade.hasLineOfSight(block1, block3));
		CHECK(block3.isInFrontOf(block1, Facing4::North));
		space.pointFeature_construct(block3, hatch, marble);
		CHECK(!area.m_opacityFacade.hasLineOfSight(block1, block3));
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 0);
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a2);
		CHECK(result2.size() == 0);
		space.pointFeature_open(block3, hatch);
		CHECK(area.m_opacityFacade.hasLineOfSight(block1, block3));
		CHECK(area.m_visionRequests.size() == 2);
		area.m_visionRequests.doStep();
		auto result3 = actors.vision_getCanSee(a1);
		CHECK(result3.size() == 1);
	}
	SUBCASE("Vision not blocked by closed hatch on low z face of vision line boundry")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block3 = Point3D::create(5, 7, 2);
		space.pointFeature_construct(block1, hatch, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block3,
			.facing=block3.getFacingTwords(block1),
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
		CHECK(actors.vision_getCanSee(a2).size() == 1);
	}
	SUBCASE("Vision not blocked by closed hatch on the same z level")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block2 = Point3D::create(5, 5, 1);
		Point3D block3 = Point3D::create(5, 7, 1);
		space.pointFeature_construct(block2, hatch, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
	}
	SUBCASE("Vision from above and below blocked by floor")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		Point3D block1 = Point3D::create(5, 5, 1);
		Point3D block2 = Point3D::create(5, 5, 2);
		Point3D block3 = Point3D::create(5, 5, 3);
		space.solid_setNot(block1);
		space.solid_setNot(block2);
		space.solid_setNot(block3);
		space.pointFeature_construct(block2, stairs, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		ActorIndex a2 = actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
		space.pointFeature_construct(block3, floor, marble);
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a1);
		CHECK(result2.size() == 0);
		area.m_visionRequests.doStep();
		auto result3 = actors.vision_getCanSee(a2);
		CHECK(result3.size() == 0);
	}
	SUBCASE("Vision from below not blocked by glass floor")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		Point3D block1 = Point3D::create(5, 5, 1);
		Point3D block2 = Point3D::create(5, 5, 2);
		Point3D block3 = Point3D::create(5, 5, 3);
		space.solid_setNot(block1);
		space.solid_setNot(block2);
		space.solid_setNot(block3);
		space.pointFeature_construct(block2, stairs, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
		space.pointFeature_construct(block3, floor, glass);
		area.m_visionRequests.doStep();
		auto result2 = actors.vision_getCanSee(a1);
		CHECK(result2.size() == 1);
	}
	SUBCASE("Vision not blocked by floor on the same z level")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D block1 = Point3D::create(5, 3, 1);
		Point3D block2 = Point3D::create(5, 5, 1);
		Point3D block3 = Point3D::create(5, 7, 1);
		space.pointFeature_construct(block2, floor, marble);
		ActorIndex a1 = actors.create({
			.species=dwarf,
			.location=block1,
			.facing=block1.getFacingTwords(block3),
		});
		actors.create({
			.species=dwarf,
			.location=block3,
		});
		area.m_visionRequests.doStep();
		auto result = actors.vision_getCanSee(a1);
		CHECK(result.size() == 1);
	}
}
TEST_CASE("Too far to see")
{
	auto marble = MaterialType::byName("marble");
	auto dwarf = AnimalSpecies::byName("dwarf");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(20,20,20);
	Actors& actors = area.getActors();
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	Point3D block1 = Point3D::create(0, 0, 1);
	Point3D block2 = Point3D::create(19, 19, 1);
	ActorIndex a1 = actors.create({
		.species=dwarf,
		.location=block1,
		.facing=block1.getFacingTwords(block2),
	});
	actors.create({
		.species=dwarf,
		.location=block2,
	});
	area.m_visionRequests.doStep();
	auto result = actors.vision_getCanSee(a1);
	CHECK(result.size() == 0);
}
