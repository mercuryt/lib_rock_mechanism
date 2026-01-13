#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/fluidGroups/fluidGroup.h"
#include "../../engine/definitions/materialType.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/space/space.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/numericTypes/types.h"
TEST_CASE("fluids smaller")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	static FluidTypeId CO2 = FluidType::byName("CO2");
	static FluidTypeId mercury = FluidType::byName("mercury");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	Support& support = space.getSupport();
	SUBCASE("Create Fluid 100")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point = Point3D::create(5, 5, 1);
		space.solid_setNot(point);
		space.fluid_add(point, CollisionVolume::create(100), water);
		CHECK(space.fluid_contains(Point3D::create(5, 5, 1), water));
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		CHECK(fluidGroup->m_fillQueue.m_set.volume() == 9);
		fluidGroup->readStep(area);
		CHECK(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->mergeStep(area);
		fluidGroup->splitStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 100);
		CHECK(!space.fluid_contains(point.south(), water));
		CHECK(!space.fluid_contains(point.east(), water));
		CHECK(!space.fluid_contains(point.north(), water));
		CHECK(!space.fluid_contains(point.west(), water));
		CHECK(!space.fluid_contains(point.below(), water));
		CHECK(!space.fluid_contains(point.above(), water));
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	}
	/*
	SUBCASE("Create Fluid 2")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 4);
		space.solid_setNot(point);
		space.fluid_add(point, CollisionVolume::create(1), water);
		space.fluid_add(point2, CollisionVolume::create(1), water);
		CHECK(space.fluid_getGroup(point,& water));
		simulation.doStep();
		simulation.doStep();
		CHECK(space.fluid_getGroup(point,& water));
		CHECK(area.m_hasFluidGroups.getUnstable().empty());
		FluidGroup& fluidGroup = *point.m_fluids.at(&water).second;
		CHECK(fluidGroup.getPoints().size() == 1);
		CHECK(fluidGroup.getPoints().contains(&point));
		CHECK(space.fluid_getTotalVolume(point) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 1);
		CHECK(fluidGroup.m_excessVolume == 1);
	}
	*/
	SUBCASE("Excess volume spawns and negitive excess despawns.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D point = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		Point3D point3 = Point3D::create(5, 5, 3);
		space.solid_setNot(point);
		space.solid_setNot(point2);
		space.fluid_add(point, Config::maxPointVolume * 2u, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 1);
		CHECK(fluidGroup->m_fillQueue.m_set.volume() == 1);
		CHECK(fluidGroup->m_fillQueue.m_set.contains(point2));
		// Step 1.
		fluidGroup->readStep(area);
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_futureAddToDrainQueue.contains(point2));
		CHECK(fluidGroup->m_futureRemoveFromFillQueue.contains(point2));
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 2);
		CHECK(fluidGroup->m_fillQueue.m_set.volume() == 9);
		CHECK(fluidGroup->m_fillQueue.m_set.contains(point3));
		CHECK(space.fluid_contains(point2, water));
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == Config::maxPointVolume);
		CHECK(fluidGroup == space.fluid_getGroup(point2, water));
		// Step 2.
		fluidGroup->readStep(area);
		CHECK(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(space.fluid_getGroup(point2, water));
		CHECK(!space.fluid_contains(Point3D::create(5, 5, 3), water));
		space.fluid_remove(point, Config::maxPointVolume, water);
		CHECK(!fluidGroup->m_stable);
		// Step 3.
		fluidGroup->readStep(area);
		CHECK(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(space.fluid_getGroup(point, water));
		CHECK(space.fluid_volumeOfTypeContains(point, water) == Config::maxPointVolume);
		CHECK(!space.fluid_getGroup(point2, water));
	}
	SUBCASE("Remove volume can destroy FluidGroups.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point = Point3D::create(5, 5, 1);
		space.solid_setNot(point);
		space.fluid_add(point, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		fluidGroup->readStep(area);
		CHECK(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		space.fluid_remove(point, CollisionVolume::create(100), water);
		CHECK(fluidGroup->m_destroy == false);
		// Step 1.
		fluidGroup->readStep(area);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 1);
		CHECK(fluidGroup->m_drainQueue.m_futureEmpty.volume() == 1);
		CHECK(fluidGroup->m_destroy == true);
	}
	SUBCASE("Flow into adjacent hole")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D destination = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		Point3D origin = Point3D::create(5, 6, 2);
		Point3D point4 = Point3D::create(5, 5, 3);
		Point3D point5 = Point3D::create(5, 6, 3);
		space.solid_setNot(destination);
		space.solid_setNot(point2);
		space.solid_setNot(origin);
		space.fluid_add(origin, Config::maxPointVolume, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 1);
		CHECK(fluidGroup->m_fillQueue.m_set.volume() == 11);
		// Step 1.
		fluidGroup->readStep(area);
		CHECK(fluidGroup->m_destroy == false);
		CHECK(fluidGroup->m_fillQueue.m_set.volume() == 11);
		CHECK(fluidGroup->m_fillQueue.m_set.contains(point2));
		CHECK(fluidGroup->m_fillQueue.m_set.contains(point5));
		// Fluid flows down. All points with z == 3 are removed from fill queue.
		CHECK(fluidGroup->m_futureRemoveFromFillQueue.contains(point5));
		CHECK(fluidGroup->m_futureRemoveFromFillQueue.contains(point4));
		CHECK(!fluidGroup->m_futureRemoveFromFillQueue.contains(point2));
		CHECK(fluidGroup->m_futureAddToFillQueue.volume() == 1);
		// Origin is no longer full so it is added to fill queue.
		CHECK(fluidGroup->m_futureAddToFillQueue.contains(origin));
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 1);
		CHECK(space.fluid_getGroup(destination, water) == fluidGroup);
		CHECK(!space.fluid_any(origin));
		CHECK(!space.fluid_any(point2));
		CHECK(!fluidGroup->m_stable);
		// Step 2.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(space.fluid_getGroup(destination, water));
		CHECK(!space.fluid_getGroup(point2, water));
		CHECK(!space.fluid_getGroup(origin, water));
		CHECK(space.fluid_volumeOfTypeContains(destination, water) == Config::maxPointVolume);
		CHECK(fluidGroup->m_fillQueue.m_set.volume() == 2);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 1);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_stable);
	}
	SUBCASE("Flow across area and then fill hole")
	{
		// Spread into square of volume 9 at first step. At second fill hole at point5.
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point = Point3D::create(5, 5, 2);
		Point3D point2a = Point3D::create(5, 6, 2);
		Point3D point2b = Point3D::create(6, 5, 2);
		Point3D point2c = Point3D::create(5, 4, 2);
		Point3D point2d = Point3D::create(4, 5, 2);
		Point3D point3 = Point3D::create(6, 6, 2);
		Point3D point4 = Point3D::create(5, 7, 2);
		Point3D point5 = Point3D::create(5, 7, 1);
		Point3D point6 = Point3D::create(5, 8, 2);
		space.fluid_add(point, Config::maxPointVolume, water);
		space.solid_setNot(point5);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 9);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point2a, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point2b, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point2c, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point2d, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point6, water) == 0);
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_excessVolume == 1);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 1);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 100);
		CHECK(fluidGroup->m_excessVolume == 0);
		CHECK(!fluidGroup->m_stable);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(fluidGroup->m_stable);
	}
	SUBCASE("FluidGroups are able to split into parts")
	{
		// Two points full of water are stacked. Adjacent two pits, on opposing sides.
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D destination1 = Point3D::create(5, 4, 1);
		Point3D destination2 = Point3D::create(5, 6, 1);
		Point3D origin1 = Point3D::create(5, 5, 2);
		Point3D origin2 = Point3D::create(5, 5, 3);
		space.solid_setNot(destination1);
		space.solid_setNot(destination2);
		space.solid_setNot(destination1.above());
		space.solid_setNot(destination2.above());
		space.solid_setNot(origin1);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		space.fluid_add(origin2, CollisionVolume::create(100), water);
		CHECK(space.fluid_getGroup(origin1, water) == space.fluid_getGroup(origin2, water));
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 2);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(destination1.above(), water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(destination2.above(), water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(destination1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(destination2, water) == 100);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		CHECK(space.fluid_getGroup(destination1, water) != space.fluid_getGroup(destination2, water));
		CHECK(area.m_hasFluidGroups.getUnstable().size() == 2);
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.getUnstable().empty());
	}
	SUBCASE("Fluid Groups merge")
	{
		// Two ponits with 100 water each are seperated by a third point in a trench.
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D origin1 = Point3D::create(5, 4, 1);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D origin2 = Point3D::create(5, 6, 1);
		space.solid_setNot(origin1);
		space.solid_setNot(point1);
		space.solid_setNot(origin2);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		space.fluid_add(origin2, CollisionVolume::create(100), water);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		CHECK(fg1 != fg2);
		// Step 1.
		area.m_hasFluidGroups.doStep();
		CHECK(fg2->m_merged);
		CHECK(fg1->m_drainQueue.m_set.volume() == 3);
		CHECK(fg1->m_excessVolume == 0);
		CHECK(fg1->m_fillQueue.m_set.volume() == 18);
		CHECK(fg1->m_drainQueue.m_set.volume() == 3);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 50);
		CHECK(fg1 == space.fluid_getGroup(origin1, water));
		CHECK(fg1 == space.fluid_getGroup(point1, water));
		CHECK(fg1 == space.fluid_getGroup(origin2, water));
		// Step 2.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		// Step 3.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 66);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 66);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 66);
		CHECK(fg1->m_excessVolume == 2);
		CHECK(fg1->m_stable);
	}
	SUBCASE("Fluid Groups merge four points")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point1 = Point3D::create(5, 4, 1);
		Point3D point2 = Point3D::create(5, 5, 1);
		Point3D point3 = Point3D::create(5, 6, 1);
		Point3D point4 = Point3D::create(5, 7, 1);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.solid_setNot(point3);
		space.solid_setNot(point4);
		space.fluid_add(point1, CollisionVolume::create(100), water);
		space.fluid_add(point4, CollisionVolume::create(100), water);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg1 = space.fluid_getGroup(point1, water);
		FluidGroup* fg2 = space.fluid_getGroup(point4, water);
		CHECK(fg1 != fg2);
		// Step 1.
		fg1->readStep(area);
		fg2->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg1->mergeStep(area);
		CHECK(fg2->m_merged);
		fg2->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 50);
		// Step 2.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->mergeStep(area);
		fg1->splitStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 50);
		CHECK(fg1->m_stable);
	}
	SUBCASE("Denser fluids sink")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		Point3D point3 = Point3D::create(5, 5, 3);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.fluid_add(point1, CollisionVolume::create(100), water);
		space.fluid_add(point2, CollisionVolume::create(100), mercury);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fgWater = space.fluid_getGroup(point1, water);
		FluidGroup* fgMercury = space.fluid_getGroup(point2, mercury);
		CHECK(fgWater != nullptr);
		CHECK(fgMercury != nullptr);
		CHECK(fgWater->m_fluidType == water);
		CHECK(fgMercury->m_fluidType == mercury);
		CHECK(fgWater->m_fillQueue.m_set.volume() == 1);
		// Step 1.
		fgWater->readStep(area);
		CHECK(fgWater->m_fillQueue.m_set.volume() == 1);
		fgMercury->readStep(area);
		fgWater->writeStep(area);
		CHECK(fgWater->m_drainQueue.m_set.volume() == 1);
		fgMercury->writeStep(area);
		fgWater->afterWriteStep(area);
		fgMercury->afterWriteStep(area);
		fgWater->splitStep(area);
		fgMercury->splitStep(area);
		CHECK(fgWater->m_fillQueue.m_set.volume() == 2);
		CHECK(fgWater->m_fillQueue.m_set.contains(point1));
		CHECK(fgWater->m_fillQueue.m_set.contains(point2));
		fgWater->mergeStep(area);
		fgMercury->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point1, mercury) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point2, mercury) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 0);
		CHECK(fgWater->m_excessVolume == 50);
		CHECK(fgMercury->m_excessVolume == 0);
		CHECK(!fgWater->m_stable);
		CHECK(!fgMercury->m_stable);
		CHECK(fgWater->m_fillQueue.m_set.volume() == 2);
		CHECK(fgWater->m_fillQueue.m_set.contains(point1));
		CHECK(fgWater->m_fillQueue.m_set.contains(point2));
		CHECK(fgMercury->m_fillQueue.m_set.volume() == 11);
		// Step 2.
		fgWater->readStep(area);
		fgMercury->readStep(area);
		fgWater->writeStep(area);
		fgMercury->writeStep(area);
		fgWater->afterWriteStep(area);
		fgMercury->afterWriteStep(area);
		fgWater->splitStep(area);
		fgMercury->splitStep(area);
		fgWater->mergeStep(area);
		fgMercury->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point1, mercury) == 100);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point2, mercury) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 0);
		CHECK(fgWater->m_excessVolume == 50);
		CHECK(fgMercury->m_excessVolume == 0);
		CHECK(!fgWater->m_stable);
		CHECK(!fgMercury->m_stable);
		CHECK(fgWater->m_fillQueue.m_set.volume() == 11);
		CHECK(fgWater->m_fillQueue.m_set.contains(point1));
		CHECK(fgWater->m_fillQueue.m_set.contains(point2));
		CHECK(fgWater->m_fillQueue.m_set.contains(point3));
		CHECK(fgWater->m_drainQueue.m_set.volume() == 1);
		CHECK(fgWater->m_drainQueue.m_set.contains(point2));
		// Step 3.
		fgWater->readStep(area);
		CHECK(fgWater->m_futureGroups.size() == 0);
		fgMercury->readStep(area);
		fgWater->writeStep(area);
		fgMercury->writeStep(area);
		fgWater->afterWriteStep(area);
		fgMercury->afterWriteStep(area);
		fgWater->splitStep(area);
		fgMercury->splitStep(area);
		fgWater->mergeStep(area);
		fgMercury->mergeStep(area);
		CHECK(fgWater->m_drainQueue.m_set.volume() == 1);
		CHECK(fgWater->m_fillQueue.m_set.volume() == 10);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point1, mercury) == 100);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(point2, mercury) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 0);
		CHECK(fgWater->m_stable);
		CHECK(fgWater->m_excessVolume == 0);
		CHECK(fgMercury->m_excessVolume == 0);
		CHECK(fgMercury->m_stable);
	}
	SUBCASE("Merge 3 groups at two point distance")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point1 = Point3D::create(5, 2, 1);
		Point3D point2 = Point3D::create(5, 3, 1);
		Point3D point3 = Point3D::create(5, 4, 1);
		Point3D point4 = Point3D::create(5, 5, 1);
		Point3D point5 = Point3D::create(5, 6, 1);
		Point3D point6 = Point3D::create(5, 7, 1);
		Point3D point7 = Point3D::create(5, 8, 1);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.solid_setNot(point3);
		space.solid_setNot(point4);
		space.solid_setNot(point5);
		space.solid_setNot(point6);
		space.solid_setNot(point7);
		space.fluid_add(point1, CollisionVolume::create(100), water);
		space.fluid_add(point4, CollisionVolume::create(100), water);
		space.fluid_add(point7, CollisionVolume::create(100), water);
		CHECK(area.m_hasFluidGroups.getAll().size() == 3);
		FluidGroup* fg1 = space.fluid_getGroup(point1, water);
		FluidGroup* fg2 = space.fluid_getGroup(point4, water);
		FluidGroup* fg3 = space.fluid_getGroup(point7, water);
		CHECK(fg1 != nullptr);
		CHECK(fg2 != nullptr);
		CHECK(fg3 != nullptr);
		CHECK(fg1->m_fluidType == water);
		CHECK(fg2->m_fluidType == water);
		CHECK(fg3->m_fluidType == water);
		// Step 1.
		fg1->readStep(area);
		fg2->readStep(area);
		fg3->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg3->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg3->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg3->splitStep(area);
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		fg3->mergeStep(area);
		// One group is merged into by the other two
		CHECK(fg1->m_merged + fg2->m_merged + fg3->m_merged == 2);
		FluidGroup* remainingGroup = (!fg1->m_merged) ? fg1 : (!fg2->m_merged) ? fg2 : fg3;
		CHECK(remainingGroup->m_drainQueue.m_set.volume() == 7);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 33);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 33);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 33);
		CHECK(space.fluid_volumeOfTypeContains(point6, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point7, water) == 50);
		// Step 2.
		remainingGroup->readStep(area);
		remainingGroup->writeStep(area);
		remainingGroup->splitStep(area);
		remainingGroup->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 42);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 42);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 42);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 42);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 42);
		CHECK(space.fluid_volumeOfTypeContains(point6, water) == 42);
		CHECK(space.fluid_volumeOfTypeContains(point7, water) == 42);
		remainingGroup->readStep(area);
		CHECK(remainingGroup->m_stable);
	}
	SUBCASE("Split test 2")
	{
		// Create a 3 x 1 trench with pits at each end.
		// One pit is a single point deep ( point4 ), the other is two points deep and includes another point on the bottom level ( points 1-3).
		// Expect a single fluid group of 3 points with a total volume of 60 to split into two groups with volume 30.
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Point3D point1 = Point3D::create(5, 4, 1);
		Point3D point2 = Point3D::create(5, 5, 1);
		Point3D point3 = Point3D::create(5, 5, 2);
		Point3D origin1 = Point3D::create(5, 5, 3);
		Point3D origin2 = Point3D::create(5, 6, 3);
		Point3D origin3 = Point3D::create(5, 7, 3);
		Point3D point4 = Point3D::create(5, 7, 2);
		space.solid_setNot(origin1);
		space.solid_setNot(origin2);
		space.solid_setNot(origin3);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.solid_setNot(point3);
		space.solid_setNot(point4);
		space.fluid_add(origin1, CollisionVolume::create(20), water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		space.fluid_add(origin2, CollisionVolume::create(20), water);
		space.fluid_add(origin3, CollisionVolume::create(20), water);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fg1->m_drainQueue.m_set.volume() == 3);
		// Step 1.
		area.m_hasFluidGroups.doStep();
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 30);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 30);
		fg1 = space.fluid_getGroup(point3, water);
		FluidGroup* fg2 = space.fluid_getGroup(point4, water);
		CHECK(fg1 != fg2);
		// Step 2.
		area.m_hasFluidGroups.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin3, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 15);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 15);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 30);
		CHECK(!fg1->m_stable);
		CHECK(fg2->m_stable);
		// Step 3.
		area.m_hasFluidGroups.doStep();
		CHECK(fg1->m_stable);
	}
	SUBCASE("Merge with group as it splits")
	{
		// A 3x1 trench with pits on each end is created.
		// One of the pits has depth one and is empty.
		// The other has depth two and contains 100 water at the bottom.
		// The 3x1 area contains 60 water.
		// Expect the 3x1 to split into two groups of 30, one of which merges with the 100.
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Point3D origin1 = Point3D::create(5, 4, 1);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		Point3D origin2 = Point3D::create(5, 5, 3);
		Point3D origin3 = Point3D::create(5, 6, 3);
		Point3D origin4 = Point3D::create(5, 7, 3);
		Point3D point3 = Point3D::create(5, 7, 2);
		space.solid_setNot(origin1);
		space.solid_setNot(origin2);
		space.solid_setNot(origin3);
		space.solid_setNot(origin4);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.solid_setNot(point3);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		space.fluid_add(origin2, CollisionVolume::create(20), water);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		space.fluid_add(origin3, CollisionVolume::create(20), water);
		space.fluid_add(origin4, CollisionVolume::create(20), water);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		CHECK(fg1->m_drainQueue.m_set.volume() == 1);
		CHECK(fg2->m_drainQueue.m_set.volume() == 3);
		CHECK(fg1 != fg2);
		// Step 1.
		area.m_hasFluidGroups.doStep();
		fg1 = space.fluid_getGroup(origin1, water);
		fg2 = space.fluid_getGroup(point3, water);
		CHECK(fg1 != fg2);
		CHECK(fg1 != nullptr);
		CHECK(fg2 != nullptr);
		CHECK(fg1->m_drainQueue.m_set.volume() == 3);
		CHECK(fg2->m_drainQueue.m_set.volume() == 1);
		CHECK(fg2->totalVolume(area) == 30);
		CHECK(fg1->totalVolume(area) == 130);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		// Step 2.
		area.m_hasFluidGroups.doStep();
		fg1 = space.fluid_getGroup(origin1, water);
		fg2 = space.fluid_getGroup(point3, water);
		CHECK(fg2->m_stable);
		CHECK(fg1->m_stable);
		CHECK(fg1->m_drainQueue.m_set.volume() == 2);
		CHECK(fg1->totalVolume(area) == 130);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 65);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 65);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 30);
	}
	SUBCASE("Merge with two groups while spliting")
	{
		// a 3x1 trench has a group with 60 water in it. Under each end is a two deep hole with 100 water at the bottom and one extra block off to the side.
		// Expect each hole to contain 130 water after one step.
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Point3D origin1 = Point3D::create(5, 4, 1);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		Point3D origin2 = Point3D::create(5, 5, 3);
		Point3D origin3 = Point3D::create(5, 6, 3);
		Point3D origin4 = Point3D::create(5, 7, 3);
		Point3D point3 = Point3D::create(5, 7, 2);
		Point3D point4 = Point3D::create(5, 7, 1);
		Point3D origin5 = Point3D::create(5, 8, 1);
		space.solid_setNot(origin1);
		space.solid_setNot(origin2);
		space.solid_setNot(origin3);
		space.solid_setNot(origin4);
		space.solid_setNot(origin5);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.solid_setNot(point3);
		space.solid_setNot(point4);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		space.fluid_add(origin2, CollisionVolume::create(20), water);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		space.fluid_add(origin3, CollisionVolume::create(20), water);
		space.fluid_add(origin4, CollisionVolume::create(20), water);
		space.fluid_add(origin5, CollisionVolume::create(100), water);
		FluidGroup* fg3 = space.fluid_getGroup(origin5, water);
		CHECK(area.m_hasFluidGroups.getAll().size() == 3);
		CHECK(fg1 != fg2);
		CHECK(fg1 != fg3);
		CHECK(fg2 != fg3);
		// Step 1.
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		fg1 = space.fluid_getGroup(origin1, water);
		fg2 = space.fluid_getGroup(origin5, water);
		CHECK(fg1->totalVolume(area) == 130);
		CHECK(fg1->m_drainQueue.m_set.volume() == 3);
		CHECK(fg2->totalVolume(area) == 130);
		CHECK(fg2->m_drainQueue.m_set.volume() == 3);
		CHECK(!fg1->m_stable);
		CHECK(!fg2->m_stable);
		// Step 2.
		area.m_hasFluidGroups.doStep();
		fg1 = space.fluid_getGroup(origin1, water);
		fg2 = space.fluid_getGroup(origin5, water);
		CHECK(fg1->totalVolume(area) == 130);
		CHECK(fg1->m_drainQueue.m_set.volume() == 2);
		CHECK(fg2->totalVolume(area) == 130);
		CHECK(fg2->m_drainQueue.m_set.volume() == 2);
		CHECK(fg1->m_stable);
		CHECK(fg2->m_stable);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 65);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 65);
		CHECK(space.fluid_volumeOfTypeContains(origin5, water) == 65);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 65);
	}
	SUBCASE("Bubbles")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Point3D origin1 = Point3D::create(5, 5, 1);
		Point3D origin2 = Point3D::create(5, 5, 2);
		Point3D origin3 = Point3D::create(5, 5, 3);
		Point3D point1 = Point3D::create(5, 5, 4);
		space.solid_setNot(origin1);
		space.solid_setNot(origin2);
		space.solid_setNot(origin3);
		space.solid_setNot(point1);
		space.fluid_add(origin1, CollisionVolume::create(100), CO2);
		space.fluid_add(origin2, CollisionVolume::create(100), water);
		space.fluid_add(origin3, CollisionVolume::create(100), water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, CO2);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		// Step 1.
		fg1->readStep(area);
		fg2->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		CHECK(fg1->m_excessVolume == 100);
		CHECK(fg1->m_disolved);
		fg2->splitStep(area);
		fg2->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, CO2) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin3, CO2) == 100);
		CHECK(fg1 == space.fluid_getGroup(origin3, CO2));
		// Step 2.
		fg1->readStep(area);
		fg2->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		CHECK(fg1->m_stable);
		fg2->removeFluid(area, CollisionVolume::create(100));
		// Step 3.
		fg2->readStep(area);
		fg2->writeStep(area);
		CHECK(!fg1->m_stable);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 0);
		fg2->splitStep(area);
		fg2->mergeStep(area);
		// Step 4.
		fg1->readStep(area);
		CHECK(fg2->m_stable);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == 100);
		// Step 5.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		CHECK(fg1->m_stable);
		CHECK(fg2->m_stable);
	}
	SUBCASE("Three liquids")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Point3D origin1 = Point3D::create(5, 5, 1);
		Point3D origin2 = Point3D::create(5, 5, 2);
		Point3D origin3 = Point3D::create(5, 5, 3);
		Point3D point1 = Point3D::create(5, 5, 4);
		space.solid_setNot(origin1);
		space.solid_setNot(origin2);
		space.solid_setNot(origin3);
		space.solid_setNot(point1);
		space.fluid_add(origin1, CollisionVolume::create(100), CO2);
		space.fluid_add(origin2, CollisionVolume::create(100), water);
		space.fluid_add(origin3, CollisionVolume::create(100), mercury);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, CO2);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		FluidGroup* fg3 = space.fluid_getGroup(origin3, mercury);
		// Step 1.
		fg1->readStep(area);
		fg2->readStep(area);
		fg3->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg3->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg3->afterWriteStep(area);
		CHECK(fg1->m_excessVolume == 100);
		CHECK(fg1->m_disolved);
		fg2->splitStep(area);
		CHECK(fg1->m_excessVolume == 50);
		fg3->splitStep(area);
		fg2->mergeStep(area);
		fg3->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, CO2) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin2, mercury) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin3, mercury) == 50);
		CHECK(fg1 == space.fluid_getGroup(origin2, CO2));
		CHECK(fg1->m_excessVolume == 50);
		// Step 2.
		fg1->readStep(area);
		fg2->readStep(area);
		fg3->readStep(area);
		CHECK(!fg3->m_stable);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg3->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg3->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg3->splitStep(area);
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		fg3->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin1, mercury) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, mercury) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin3, CO2) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin3, mercury) == 0);
		CHECK(space.fluid_getTotalVolume(point1) == 0);
		CHECK(fg2->m_excessVolume == 50);
		CHECK(fg1->m_excessVolume == 0);
		// Step 3.
		fg1->readStep(area);
		fg2->readStep(area);
		fg3->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg3->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg3->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg3->splitStep(area);
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		fg3->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, mercury) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin2, mercury) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == 50);
		CHECK(space.fluid_volumeOfTypeContains(origin3, CO2) == 50);
		CHECK(space.fluid_getTotalVolume(point1) == 0);
		CHECK(fg1->m_excessVolume == 0);
		CHECK(fg2->m_excessVolume == 50);
		// Step 4.
		fg1->readStep(area);
		fg2->readStep(area);
		fg3->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg3->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg3->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg3->splitStep(area);
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		fg3->mergeStep(area);
		CHECK(fg2->m_stable);
		CHECK(space.fluid_volumeOfTypeContains(origin1, mercury) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(origin3, CO2) == 50);
		CHECK(space.fluid_getTotalVolume(point1) == 0);
		CHECK(fg1->m_excessVolume == 50);
		CHECK(fg2->m_excessVolume == 0);
		CHECK(fg2->m_stable);
		CHECK(fg3->m_stable);
		// Step 5.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		CHECK(fg1->m_stable);
		CHECK(fg3->m_stable);
	}
	SUBCASE("Set not solid")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D origin1 = Point3D::create(5, 5, 1);
		Point3D point1 = Point3D::create(5, 6, 1);
		Point3D point2 = Point3D::create(5, 7, 1);
		space.solid_setNot(origin1);
		space.solid_setNot(point2);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		CHECK(fg1 != nullptr);
		// Step 1.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 100);
		CHECK(fg1->m_stable);
		// Step 2.
		space.solid_setNot(point1);
		CHECK(!fg1->m_stable);
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 50);
		// Step .
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 33);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 33);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 33);
		CHECK(!fg1->m_stable);
		CHECK(fg1->m_excessVolume == 1);
	}
	SUBCASE("Set solid")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D origin1 = Point3D::create(5, 5, 1);
		Point3D point1 = Point3D::create(5, 6, 1);
		Point3D point2 = Point3D::create(5, 7, 1);
		space.solid_setNot(origin1);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		// Step 1.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 50);
		CHECK(fg1->m_excessVolume == 0);
		space.solid_set(point1, marble, false);
	}
	SUBCASE("Set solid and split")
	{
		// Create a 3x1 trench and put 100 water in the middle point.
		// Allow it to spread out to 33 in each point with 1 excessVolume.
		// Set the middle point solid and verify that the fluid group splits into two groups, each containing 50.
		//
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D origin1 = Point3D::create(5, 6, 1);
		Point3D point2 = Point3D::create(5, 7, 1);
		space.solid_setNot(point1);
		space.solid_setNot(origin1);
		space.solid_setNot(point2);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		// Step 1.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 33);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 33);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 33);
		CHECK(fg1->m_excessVolume == 1);
		// Step 2.
		space.solid_set(origin1, marble, false);
		CHECK(space.solid_isAny(origin1));
		CHECK(!fg1->m_fillQueue.m_set.contains(origin1));
		CHECK(fg1->m_potentiallySplitFromSyncronusStep.size() == 2);
		CHECK(fg1->m_potentiallySplitFromSyncronusStep.contains(point1));
		CHECK(fg1->m_potentiallySplitFromSyncronusStep.contains(point2));
		CHECK(fg1->m_excessVolume == 34);
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 50);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 50);
		fg1->splitStep(area);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg2 = &area.m_hasFluidGroups.getAll().back();
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		//Step 3.
		assert(area.m_hasFluidGroups.getAll().size() == 2);
		fg1 = space.fluid_getGroup(point1, water);
		fg2 = space.fluid_getGroup(point2, water);
		if(!fg1->m_stable)
			fg1->readStep(area);
		if(!fg2->m_stable)
			fg2->readStep(area);
		CHECK(fg2->m_stable);
		CHECK(fg1->m_stable);
	}
	SUBCASE("Cave in falls in fluid and pistons it up")
	{
		if constexpr(!Config::fluidPiston)
			return
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		space.solid_setNot(point1);
		space.fluid_add(point1, CollisionVolume::create(100), water);
		space.solid_set(point2, marble, false);
		support.maybeFall({point2, point2});
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		support.doStep(area);
		CHECK(area.m_hasFluidGroups.getUnstable().size() == 1);
		CHECK(space.fluid_getTotalVolume(point1) == 0);
		CHECK(space.solid_get(point1) == marble);
		CHECK(!space.solid_isAny(point2));
		CHECK(fluidGroup->m_excessVolume == 100);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 0);
		CHECK(fluidGroup->m_fillQueue.m_set.volume() == 1);
		CHECK(fluidGroup->m_fillQueue.m_set.contains(point2));
		CHECK(space.fluid_canEnterEver(point2));
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		support.doStep(area);
		CHECK(space.fluid_getTotalVolume(point2) == 100);
		CHECK(fluidGroup->m_excessVolume == 0);
		CHECK(fluidGroup->m_stable == false);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
	}
	SUBCASE("Test mist")
	{
		simulation.m_step = Step::create(1);
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		Point3D point3 = Point3D::create(5, 5, 3);
		Point3D point4 = Point3D::create(5, 5, 4);
		Point3D point5 = Point3D::create(5, 6, 3);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.fluid_add(point3, CollisionVolume::create(100), water);
		space.fluid_add(point4, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		// Step 1.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		CHECK(space.fluid_getMist(point5) == water);
		// Several steps.
		while(simulation.m_step < 11)
		{
			if(!fluidGroup->m_stable)
			{
				fluidGroup->readStep(area);
				fluidGroup->writeStep(area);
				fluidGroup->afterWriteStep(area);
				fluidGroup->splitStep(area);
				fluidGroup->mergeStep(area);
			}
			++simulation.m_step;
		}
		simulation.m_eventSchedule.doStep(Step::create(11));
		CHECK(space.fluid_getMist(point5).empty());
	}
}
TEST_CASE("area larger")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(20,20,20);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	SUBCASE("Flow across flat area double stack")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin1 = Point3D::create(10, 10, 1);
		Point3D origin2 = Point3D::create(10, 10, 2);
		Point3D point1 = Point3D::create(10, 11, 1);
		Point3D point2 = Point3D::create(11, 11, 1);
		Point3D point3 = Point3D::create(10, 12, 1);
		Point3D point4 = Point3D::create(10, 13, 1);
		Point3D point5 = Point3D::create(10, 14, 1);
		Point3D point6 = Point3D::create(15, 10, 1);
		Point3D point7 = Point3D::create(16, 10, 1);
		space.fluid_add(origin1, CollisionVolume::create(100), water);
		space.fluid_add(origin2, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 2);
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 9);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 21);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 21);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 21);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 0);
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_excessVolume == 11);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 25);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 7);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 7);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 7);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 7);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 0);
		CHECK(fluidGroup->m_excessVolume == 25);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 49);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 4);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 4);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 4);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 4);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 4);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 0);
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_excessVolume == 4);
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 81);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point6, water) == 0);
		CHECK(fluidGroup->m_excessVolume == 38);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 121);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point6, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point7, water) == 0);
		CHECK(fluidGroup->m_excessVolume == 79);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 169);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point7, water) == 1);
		CHECK(fluidGroup->m_excessVolume == 31);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_stable);
	}
	SUBCASE("Flow across flat area")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D point = Point3D::create(10, 10, 1);
		Point3D point2 = Point3D::create(10, 12, 1);
		Point3D point3 = Point3D::create(11, 11, 1);
		Point3D point4 = Point3D::create(10, 13, 1);
		Point3D point5 = Point3D::create(10, 14, 1);
		Point3D point6 = Point3D::create(10, 15, 1);
		space.fluid_add(point, Config::maxPointVolume, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		//Step 1.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 9);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 0);
		for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(point))
			if(adjacent.z() == 1)
				CHECK(space.fluid_volumeOfTypeContains(adjacent, water) == 11);
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_excessVolume == 1);
		//Step 2.
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 25);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 3);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 3);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 3);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 0);
		CHECK(fluidGroup->m_excessVolume == 25);
		//Step 3.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 49);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 2);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 0);
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_excessVolume == 2);
		//Step 4.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_drainQueue.m_set.volume() == 81);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point4, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point5, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point6, water) == 0);
		CHECK(fluidGroup->m_excessVolume == 19);
		//Step 5.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_stable);
	}
}
TEST_CASE("fluids multi scale")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	static FluidTypeId CO2 = FluidType::byName("CO2");
	static FluidTypeId mercury = FluidType::byName("mercury");
	static FluidTypeId lava = FluidType::byName("lava");
	Simulation simulation;
	auto trenchTest2Fluids = [&](int scaleL, int scaleW, Step steps)
	{
		int maxX = scaleL + 2;
		int maxY = scaleW + 2;
		int maxZ = scaleW + 1;
		int halfMaxX = maxX / 2;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Space& space = area.getSpace();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(halfMaxX - 1, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		Point3D CO2_1 = Point3D::create(halfMaxX, 1, 1);
		Point3D CO2_2 = Point3D::create(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
		CHECK(!fgWater->m_merged);
		CHECK(!fgCO2->m_merged);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = 1 + (maxZ - 1) / 2;
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(fgWater->m_stable);
		CHECK(fgWater->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgWater->totalVolume(area) == totalVolume);
		CHECK(fgCO2->m_stable);
		CHECK(fgCO2->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgCO2->totalVolume(area) == totalVolume);
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, 1), water));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, 1), water));
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, maxZ - 1), CO2));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, maxZ - 1), CO2));
	};
	SUBCASE("trench test 2 fluids scale 2-1")
	{
		trenchTest2Fluids(2, 1, Step::create(8));
	}
	SUBCASE("trench test 2 fluids scale 4-1")
	{
		trenchTest2Fluids(4, 1, Step::create(12));
	}
	SUBCASE("trench test 2 fluids scale 40-1")
	{
		trenchTest2Fluids(40, 1, Step::create(30));
	}
	SUBCASE("trench test 2 fluids scale 20-5")
	{
		trenchTest2Fluids(20, 5, Step::create(20));
	}
	auto trenchTest3Fluids = [&](int scaleL, int scaleW, Step steps)
	{
		int maxX = scaleL + 2;
		int maxY = scaleW + 2;
		int maxZ = scaleW + 1;
		int thirdMaxX = maxX / 3;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Space& space = area.getSpace();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(thirdMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		Point3D CO2_1 = Point3D::create(thirdMaxX + 1, 1, 1);
		Point3D CO2_2 = Point3D::create((thirdMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Lava
		Point3D lava1 = Point3D::create((thirdMaxX * 2) + 1, 1, 1);
		Point3D lava2 = Point3D::create(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
		CHECK(area.m_hasFluidGroups.getAll().size() == 3);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = space.fluid_getGroup(lava1, lava);
		simulation.m_step = Step::create(1);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = std::max(1, maxZ / 3);
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(fgWater->m_stable);
		CHECK(fgWater->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgWater->totalVolume(area) == totalVolume);
		CHECK(fgCO2->m_stable);
		CHECK(fgCO2->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgCO2->totalVolume(area) == totalVolume);
		CHECK(fgLava->m_stable);
		CHECK(fgLava->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgLava->totalVolume(area) == totalVolume);
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, 1), lava));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, 1), lava));
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, maxZ - 1), CO2));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, maxZ - 1), CO2));
	};
	SUBCASE("trench test 3 fluids scale 3-1")
	{
		trenchTest3Fluids(3, 1, Step::create(10));
	}
	SUBCASE("trench test 3 fluids scale 9-1")
	{
		trenchTest3Fluids(9, 1, Step::create(20));
	}
	SUBCASE("trench test 3 fluids scale 3-3")
	{
		trenchTest3Fluids(3, 3, Step::create(10));
	}
	SUBCASE("trench test 3 fluids scale 18-3")
	{
		trenchTest3Fluids(18, 3, Step::create(30));
	}
	auto trenchTest4Fluids = [&](int scaleL, int scaleW, Step steps)
	{
		int maxX = scaleL + 2;
		int maxY = scaleW + 2;
		int maxZ = scaleW + 1;
		int quarterMaxX = maxX / 4;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Space& space = area.getSpace();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(quarterMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		Point3D CO2_1 = Point3D::create(quarterMaxX + 1, 1, 1);
		Point3D CO2_2 = Point3D::create((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Lava
		Point3D lava1 = Point3D::create((quarterMaxX * 2) + 1, 1, 1);
		Point3D lava2 = Point3D::create(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
		// Mercury
		Point3D mercury1 = Point3D::create((quarterMaxX * 3) + 1, 1, 1);
		Point3D mercury2 = Point3D::create(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
		CHECK(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = space.fluid_getGroup(lava1, lava);
		FluidGroup* fgMercury = space.fluid_getGroup(mercury1, mercury);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			space.prepareRtrees();
			fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
			if(fgMercury != nullptr)
				CHECK(fgMercury->totalVolume(area) == totalVolume);
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = std::max(1, maxZ / 4);
		int expectedBlocks = totalBlocks2D * expectedHeight;
		fgWater = areaBuilderUtil::getFluidGroup(area, water);
		CHECK(fgWater != nullptr);
		fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
		CHECK(fgCO2 != nullptr);
		fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
		CHECK(fgMercury != nullptr);
		fgLava = areaBuilderUtil::getFluidGroup(area, lava);
		CHECK(fgLava != nullptr);
		CHECK(fgWater->m_stable);
		CHECK(fgWater->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgWater->totalVolume(area) == totalVolume);
		CHECK(fgCO2->m_stable);
		CHECK(fgCO2->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgCO2->totalVolume(area) == totalVolume);
		CHECK(fgLava->m_stable);
		CHECK(fgLava->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgLava->totalVolume(area) == totalVolume);
		CHECK(fgMercury->m_stable);
		CHECK(fgMercury->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgMercury->totalVolume(area) == totalVolume);
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, 1), mercury));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, 1), mercury));
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, maxZ - 1), CO2));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, maxZ - 1), CO2));
		CHECK(area.m_hasFluidGroups.getAll().size() == 4);
		CHECK(area.m_hasFluidGroups.getUnstable().empty());
	};
	SUBCASE("trench test 4 fluids scale 4-1")
	{
		trenchTest4Fluids(4, 1, Step::create(10));
	}
	SUBCASE("trench test 4 fluids scale 4-2")
	{
		trenchTest4Fluids(4, 2, Step::create(10));
	}
	SUBCASE("trench test 4 fluids scale 4-4")
	{
		trenchTest4Fluids(4, 4, Step::create(15));
	}
	SUBCASE("trench test 4 fluids scale 4-8")
	{
		trenchTest4Fluids(4, 8, Step::create(17));
	}
	SUBCASE("trench test 4 fluids scale 8-8")
	{
		trenchTest4Fluids(8, 8, Step::create(20));
	}
	SUBCASE("trench test 4 fluids scale 16-4")
	{
		trenchTest4Fluids(16, 4, Step::create(26));
	}
	auto trenchTest2FluidsMerge = [&](int scaleL, int scaleW, Step steps)
	{
		int maxX = scaleL + 2;
		int maxY = scaleW + 2;
		int maxZ = scaleW + 1;
		int quarterMaxX = maxX / 4;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Space& space = area.getSpace();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(quarterMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		Point3D CO2_1 = Point3D::create(quarterMaxX + 1, 1, 1);
		Point3D CO2_2 = Point3D::create((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Water
		Point3D water3 = Point3D::create((quarterMaxX * 2) + 1, 1, 1);
		Point3D water4 = Point3D::create(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water3, water4, water);
		// CO2
		Point3D CO2_3 = Point3D::create((quarterMaxX * 3) + 1, 1, 1);
		Point3D CO2_4 = Point3D::create(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_3, CO2_4, CO2);
		CHECK(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		CollisionVolume totalVolume = fgWater->totalVolume(area) * 2u;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = std::max(1, maxZ / 2);
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(area.m_hasFluidGroups.getUnstable().empty());
		fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(water2, CO2);
		CHECK(fgWater->totalVolume(area) == totalVolume);
		CHECK(fgCO2->totalVolume(area) == totalVolume);
		CHECK(fgWater->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgCO2->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(area.m_hasFluidGroups.getAll().size() == 2);
	};
	SUBCASE("trench test 2 fluids merge scale 4-1")
	{
		trenchTest2FluidsMerge(4, 1, Step::create(6));
	}
	SUBCASE("trench test 2 fluids merge scale 4-4")
	{
		trenchTest2FluidsMerge(4, 4, Step::create(6));
	}
	SUBCASE("trench test 2 fluids merge scale 8-4")
	{
		trenchTest2FluidsMerge(8, 4, Step::create(8));
	}
	SUBCASE("trench test 2 fluids merge scale 16-4")
	{
		trenchTest2FluidsMerge(16, 4, Step::create(12));
	}
	auto trenchTest3FluidsMerge = [&](int scaleL, int scaleW, Step steps)
	{
		int maxX = scaleL + 2;
		int maxY = scaleW + 2;
		int maxZ = scaleW + 1;
		int quarterMaxX = maxX / 4;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Space& space = area.getSpace();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(quarterMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		Point3D CO2_1 = Point3D::create(quarterMaxX + 1, 1, 1);
		Point3D CO2_2 = Point3D::create((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Mercury
		Point3D mercury1 = Point3D::create((quarterMaxX * 2) + 1, 1, 1);
		Point3D mercury2 = Point3D::create(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
		// CO2
		Point3D CO2_3 = Point3D::create((quarterMaxX * 3) + 1, 1, 1);
		Point3D CO2_4 = Point3D::create(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_3, CO2_4, CO2);
		CHECK(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		CollisionVolume totalVolumeWater = fgWater->totalVolume(area);
		CollisionVolume totalVolumeMercury = totalVolumeWater;
		CollisionVolume totalVolumeCO2 = totalVolumeWater * 2u;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = std::max(1, maxZ / 4);
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(area.m_hasFluidGroups.getUnstable().empty());
		fgWater = areaBuilderUtil::getFluidGroup(area, water);
		FluidGroup* fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
		FluidGroup* fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
		CHECK(fgWater != nullptr);
		CHECK(fgCO2 != nullptr);
		CHECK(fgMercury != nullptr);
		CHECK(fgWater->totalVolume(area) == totalVolumeWater);
		CHECK(fgCO2->totalVolume(area) == totalVolumeCO2);
		CHECK(fgMercury->totalVolume(area) == totalVolumeMercury);
		CHECK(fgWater->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK((fgCO2->m_drainQueue.m_set.volume() == expectedBlocks || fgCO2->m_drainQueue.m_set.volume() == expectedBlocks * 2));
		CHECK(fgMercury->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgWater->m_stable);
		CHECK(fgCO2->m_stable);
		CHECK(fgMercury->m_stable);
		CHECK(area.m_hasFluidGroups.getAll().size() == 3);
	};
	SUBCASE("trench test 3 fluids merge scale 4-1")
	{
		trenchTest3FluidsMerge(4, 1, Step::create(8));
	}
	SUBCASE("trench test 3 fluids merge scale 4-4")
	{
		trenchTest3FluidsMerge(4, 4, Step::create(10));
	}
	SUBCASE("trench test 3 fluids merge scale 8-4")
	{
		trenchTest3FluidsMerge(8, 4, Step::create(15));
	}
	SUBCASE("trench test 3 fluids merge scale 16-4")
	{
		trenchTest3FluidsMerge(16, 4, Step::create(24));
	}
}
TEST_CASE("four fluids multi scale")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	static FluidTypeId CO2 = FluidType::byName("CO2");
	static FluidTypeId mercury = FluidType::byName("mercury");
	static FluidTypeId lava = FluidType::byName("lava");
	Simulation simulation;
	auto fourFluidsTest = [&](int scale, Step steps)
	{
		int maxX = (scale * 2) + 2;
		int maxY = (scale * 2) + 2;
		int maxZ = (scale * 1) + 1;
		int halfMaxX = maxX / 2;
		int halfMaxY = maxY / 2;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Space& space = area.getSpace();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		std::vector<FluidGroup*> newlySplit;
		// Water is at 0,0
		Point3D water1 = Point3D::create(1, 1, 1);
		Point3D water2 = Point3D::create(halfMaxX - 1, halfMaxY - 1, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2 is at 0,1
		Point3D CO2_1 = Point3D::create(1, halfMaxY, 1);
		Point3D CO2_2 = Point3D::create(halfMaxX - 1, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Lava is at 1,0
		Point3D lava1 = Point3D::create(halfMaxX, 1, 1);
		Point3D lava2 = Point3D::create(maxX - 2, halfMaxY - 1, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
		// Mercury is at 1,1
		Point3D mercury1 = Point3D::create(halfMaxX, halfMaxY, 1);
		Point3D mercury2 = Point3D::create(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
		CHECK(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = space.fluid_getGroup(lava1, lava);
		FluidGroup* fgMercury = space.fluid_getGroup(mercury1, mercury);
		CHECK(!fgWater->m_merged);
		CHECK(!fgCO2->m_merged);
		CHECK(!fgLava->m_merged);
		CHECK(!fgMercury->m_merged);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = ((maxZ - 2) / 4) + 1;
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(area.m_hasFluidGroups.getAll().size() == 4);
		fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
		fgWater = areaBuilderUtil::getFluidGroup(area, water);
		fgLava = areaBuilderUtil::getFluidGroup(area, lava);
		fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
		CHECK(fgWater != nullptr);
		CHECK(fgWater->m_stable);
		CHECK(fgWater->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgWater->totalVolume(area) == totalVolume);
		CHECK(fgCO2 != nullptr);
		CHECK(fgCO2->m_stable);
		CHECK(fgCO2->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgCO2->totalVolume(area) == totalVolume);
		CHECK(fgLava != nullptr);
		CHECK(fgLava->m_stable);
		CHECK(fgLava->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgLava->totalVolume(area) == totalVolume);
		CHECK(fgMercury != nullptr);
		CHECK(fgMercury->m_stable);
		CHECK(fgMercury->m_drainQueue.m_set.volume() == expectedBlocks);
		CHECK(fgMercury->totalVolume(area) == totalVolume);
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, 1), mercury));
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, maxZ - 1), CO2));
	};
	SUBCASE("four fluids scale 2")
	{
		fourFluidsTest(2, Step::create(11));
	}
	// Scale 3 doesn't work due to rounding issues with expectedBlocks.
	SUBCASE("four fluids scale 4")
	{
		fourFluidsTest(4, Step::create(21));
	}
	SUBCASE("four fluids scale 5")
	{
		fourFluidsTest(5, Step::create(28));
	}
	SUBCASE("four fluids scale 6")
	{
		fourFluidsTest(6, Step::create(30));
	}
}
