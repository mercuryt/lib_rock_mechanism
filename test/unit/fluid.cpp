#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/fluid/fluidGroup.h"
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
TEST_CASE("fluidsSmaller")
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
	int64_t maxPointVolume = Config::maxPointVolume.get();
	SUBCASE("Create Fluid maxPointVolume")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point = Point3D::create(5, 5, 1);
		space.solid_setNot(point);
		space.fluid_add(point.toSet(), maxPointVolume, water);
		CHECK(space.fluid_contains(Point3D::create(5, 5, 1), water));
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		CHECK(!fluidGroup->m_stable);
		simulation.doStep();
		CHECK(fluidGroup->m_stable);
		CHECK(fluidGroup->m_volume == maxPointVolume);
		CHECK(fluidGroup->m_occupied.volume() == 1);
		CHECK(fluidGroup->m_occupied.contains(point));
		CHECK(space.fluid_queryGetAll(point.inflated({1})).size() == 1);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == maxPointVolume);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
	}
	SUBCASE("Excess volume spawns and negitive excess despawns.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D point = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		space.solid_setNot(point);
		space.solid_setNot(point2);
		space.fluid_add(point.toSet(), Config::maxPointVolume.get() * 2, water);
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_occupied.volume() == 1);
		// Step 1.
		simulation.doStep();
		CHECK(!fluidGroup->m_stable);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_occupied.volume() == 2);
		CHECK(space.fluid_contains(point2, water));
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == Config::maxPointVolume);
		CHECK(fluidGroup == space.fluid_getGroup(point2, water));
		// Step 2.
		simulation.doStep();
		CHECK(space.fluid_getGroup(point2, water) != nullptr);
		CHECK(!space.fluid_contains(Point3D::create(5, 5, 3), water));
		space.fluid_remove(point.toSet(), Config::maxPointVolume.get(), water);
		CHECK(!fluidGroup->m_stable);
		// Step 3.
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(space.fluid_getGroup(point, water));
		CHECK(space.fluid_volumeOfTypeContains(point, water) == Config::maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 0);
		CHECK(!space.fluid_getGroup(point2, water));
		CHECK(fluidGroup->m_occupied.volume() == 1);
	}
	SUBCASE("Remove volume can destroy FluidGroups.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point = Point3D::create(5, 5, 1);
		space.solid_setNot(point);
		space.fluid_add(point.toSet(), maxPointVolume, water);
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		space.fluid_remove(point.toSet(), maxPointVolume, water);
		CHECK(area.m_hasFluidGroups.m_groups.empty());
	}
	SUBCASE("Flow into adjacent hole")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D destination = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		Point3D origin = Point3D::create(5, 6, 2);
		space.solid_setNot(destination);
		space.solid_setNot(point2);
		space.solid_setNot(origin);
		space.fluid_add(origin.toSet(), Config::maxPointVolume.get(), water);
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		CHECK(!fluidGroup->m_stable);
		CHECK(fluidGroup->m_occupied.volume() == 1);
		// Step 1.
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_occupied.volume() == 1);
		CHECK(space.fluid_getGroup(destination, water) == fluidGroup);
		CHECK(!space.fluid_any(origin));
		CHECK(!space.fluid_any(point2));
		CHECK(!fluidGroup->m_stable);
		// Step 2.
		simulation.doStep();
		CHECK(space.fluid_getGroup(destination, water));
		CHECK(!space.fluid_getGroup(point2, water));
		CHECK(!space.fluid_getGroup(origin, water));
		CHECK(space.fluid_volumeOfTypeContains(destination, water) == Config::maxPointVolume);
		CHECK(fluidGroup->m_occupied.volume() == 1);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_stable);
	}
	SUBCASE("Flow across area and then fill hole")
	{
		// Spread into square of volume 9 at first step. At second fill hole at point5.
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D point = Point3D::create(5, 5, 2);
		Point3D point2 = Point3D::create(5, 7, 1);
		space.fluid_add(point.toSet(), Config::maxPointVolume.get(), water);
		space.solid_setNot(point2);
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_occupied.volume() == 9);
		CHECK(!fluidGroup->m_stable);
		simulation.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 1);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == maxPointVolume);
		CHECK(!fluidGroup->m_stable);
		simulation.doStep();
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
		space.fluid_add(origin1.toSet(), maxPointVolume, water);
		space.fluid_add(origin2.toSet(), maxPointVolume, water);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		CHECK(fluidGroup->m_occupied.volume() == 5);
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(destination1.above(), water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(destination2.above(), water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(destination1, water) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(destination2, water) == maxPointVolume);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		CHECK(space.fluid_getGroup(destination1, water) != space.fluid_getGroup(destination2, water));
		area.m_hasFluidGroups.doStep();
		CHECK(!area.m_hasFluidGroups.hasUnstable());
	}
	SUBCASE("Fluid Groups merge")
	{
		// Two ponits with maxPointVolume water each are seperated by a third point in a trench.
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Point3D origin1 = Point3D::create(5, 4, 1);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D origin2 = Point3D::create(5, 6, 1);
		space.solid_setNot(origin1);
		space.solid_setNot(point1);
		space.solid_setNot(origin2);
		space.fluid_add(origin1.toSet(), maxPointVolume, water);
		space.fluid_add(origin2.toSet(), maxPointVolume, water);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		CHECK(fg1 != fg2);
		// Step 1.
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		fg1 = space.fluid_getGroup(origin1, water);
		CHECK(fg1->m_occupied.volume() == 3);
		CHECK(fg1->m_volume == maxPointVolume * 2);
		CHECK(fg1 == space.fluid_getGroup(origin1, water));
		CHECK(fg1 == space.fluid_getGroup(point1, water));
		CHECK(fg1 == space.fluid_getGroup(origin2, water));
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 66);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 66);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 66);
		simulation.doStep();
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
		space.fluid_add(point1.toSet(), maxPointVolume, water);
		space.fluid_add(point4.toSet(), maxPointVolume, water);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		FluidGroup* fg1 = space.fluid_getGroup(point1, water);
		FluidGroup* fg2 = space.fluid_getGroup(point4, water);
		CHECK(fg1 != fg2);
		// Step 1.
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		fg1 = &area.m_hasFluidGroups.m_groups[0];
		CHECK(fg1->m_volume == maxPointVolume * 2);
		CHECK(fg1->m_occupied.volume() == 4);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == maxPointVolume / 2);
		// Step 2.
		simulation.doStep();
		CHECK(fg1->m_stable);
	}
	SUBCASE("Denser fluids sink")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Point3D point1 = Point3D::create(5, 5, 1);
		Point3D point2 = Point3D::create(5, 5, 2);
		space.solid_setNot(point1);
		space.solid_setNot(point2);
		space.fluid_add(point1.toSet(), maxPointVolume, water);
		space.fluid_add(point2.toSet(), maxPointVolume, mercury);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		FluidGroup* fgWater = space.fluid_getGroup(point1, water);
		FluidGroup* fgMercury = space.fluid_getGroup(point2, mercury);
		CHECK(fgWater != nullptr);
		CHECK(fgMercury != nullptr);
		CHECK(fgWater->m_fluidType == water);
		CHECK(fgMercury->m_fluidType == mercury);
		// Step 1. Both fluids occupy point1.
		simulation.doStep();
		CHECK(fgWater->m_occupied.volume() == 1);
		CHECK(fgMercury->m_occupied.volume() == 1);
		CHECK(fgWater->m_occupied.contains(point1));
		CHECK(fgMercury->m_occupied.contains(point1));
		CHECK(!fgWater->m_stable);
		CHECK(!fgMercury->m_stable);
		// Step 2. Water is displaced to point2.
		simulation.doStep();
		CHECK(fgWater->m_occupied.volume() == 1);
		CHECK(fgMercury->m_occupied.volume() == 1);
		CHECK(fgWater->m_occupied.contains(point2));
		CHECK(fgMercury->m_occupied.contains(point1));
		CHECK(!fgWater->m_stable);
		CHECK(fgMercury->m_stable);
		CHECK(fgWater->m_flowingUp);
		// Step 3. Both are stable.
		simulation.doStep();
		CHECK(fgWater->m_occupied.volume() == 1);
		CHECK(fgMercury->m_occupied.volume() == 1);
		CHECK(fgWater->m_occupied.contains(point2));
		CHECK(fgMercury->m_occupied.contains(point1));
		CHECK(fgWater->m_stable);
		CHECK(fgMercury->m_stable);
		CHECK(!fgWater->m_flowingUp);
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
		space.fluid_add(point1.toSet(), maxPointVolume, water);
		space.fluid_add(point4.toSet(), maxPointVolume, water);
		space.fluid_add(point7.toSet(), maxPointVolume, water);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 3);
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
		simulation.doStep();
		// One group is merged into by the other two
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		FluidGroup* remainingGroup = &area.m_hasFluidGroups.m_groups[0];
		CHECK(remainingGroup->m_occupied.volume() == 7);
		CHECK(remainingGroup->m_volume == Config::maxPointVolume.get() * 3);
		// Step 2.
		simulation.doStep();
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
		space.fluid_add(Cuboid::create(origin1, origin3).toSet(), 60, water);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		CHECK(fg1->m_occupied.volume() == 3);
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
		fg1 = space.fluid_getGroup(point1, water);
		fg2 = space.fluid_getGroup(point4, water);
		CHECK(fg1->m_stable);
		CHECK(fg2->m_stable);
	}
	SUBCASE("Merge with group as it splits")
	{
		// A 3x1 trench with pits on each end is created.
		// One of the pits has depth one and is empty.
		// The other has depth two and contains maxPointVolume water at the bottom.
		// The 3x1 area (origin2:origin4) contains maxPointVolume water.
		// Expect the 3x1 to split into two groups of half maxPointVolume, one of which merges with the maxPointVolume in origin1.
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
		space.fluid_add(Cuboid::create(origin2, origin4).toSet(), maxPointVolume, water);
		space.fluid_add(origin1.toSet(), maxPointVolume, water);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		FluidGroup* fg1 = &area.m_hasFluidGroups.m_groups[0];
		FluidGroup* fg2 = &area.m_hasFluidGroups.m_groups[1];
		CHECK(fg1->m_occupied.volume() == 3);
		CHECK(fg2->m_occupied.volume() == 1);
		// Step 1.
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		fg1 = space.fluid_getGroup(origin1, water);
		fg2 = space.fluid_getGroup(point3, water);
		CHECK(fg1 != fg2);
		CHECK(fg1 != nullptr);
		CHECK(fg2 != nullptr);
		CHECK(fg1->m_occupied.volume() == 3);
		CHECK(fg2->m_occupied.volume() == 1);
		CHECK(fg2->m_volume == maxPointVolume / 2);
		CHECK(fg1->m_volume == maxPointVolume * 1.5);
		// Step 2.
		area.m_hasFluidGroups.doStep();
		fg1 = space.fluid_getGroup(origin1, water);
		CHECK(fg2->m_stable);
		fg2 = space.fluid_getGroup(point3, water);
		CHECK(fg1->m_occupied.volume() == 2);
		CHECK(fg1->m_volume == maxPointVolume * 1.5);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == (maxPointVolume * 3) / 4);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == (maxPointVolume * 3) / 4);
		CHECK(space.fluid_volumeOfTypeContains(point3, water) == maxPointVolume / 2);
		// Step 3.
		simulation.doStep();
		CHECK(fg1->m_stable);
	}
	SUBCASE("Merge with two groups while spliting")
	{
		// a 3x1 trench has a group with maxPointVolume water in it. Under each end is a two deep hole with maxPointVolume water at the bottom and one extra block off to the side.
		// Expect each hole to contain maxPointVolume * 1.5 water after one step.
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
		space.fluid_add(origin1.toSet(), maxPointVolume, water);
		space.fluid_add(origin5.toSet(), maxPointVolume, water);
		space.fluid_add(Cuboid::create(origin2, origin4).toSet(), maxPointVolume, water);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 3);
		// Step 1.
		simulation.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		FluidGroup* fg2 = space.fluid_getGroup(origin5, water);
		CHECK(fg1->m_volume == maxPointVolume * 1.5);
		CHECK(fg2->m_volume == maxPointVolume * 1.5);
		CHECK(fg1->m_occupied.volume() == 3);
		CHECK(fg2->m_occupied.volume() == 3);
		CHECK(!fg1->m_stable);
		CHECK(!fg2->m_stable);
		// Step 2.
		simulation.doStep();
		fg1 = space.fluid_getGroup(origin1, water);
		fg2 = space.fluid_getGroup(origin5, water);
		CHECK(fg1->m_volume == maxPointVolume * 1.5);
		CHECK(fg2->m_volume == maxPointVolume * 1.5);
		CHECK(fg1->m_occupied.volume() == 2);
		CHECK(fg2->m_occupied.volume() == 2);
		// Step 3.
		simulation.doStep();
		CHECK(fg1->m_stable);
		CHECK(fg2->m_stable);
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
		space.fluid_add(origin1.toSet(), maxPointVolume, CO2);
		space.fluid_add(origin2.toSet(), maxPointVolume, water);
		space.fluid_add(origin3.toSet(), maxPointVolume, water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, CO2);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		// Step 1. Water flows down into space occupied by CO2.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, CO2) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == 0);
		// Step 2. Water is stable, CO2 flows up.
		simulation.doStep();
		fg1 = space.fluid_getGroup(origin2, CO2);
		fg2 = space.fluid_getGroup(origin1, water);
		CHECK(fg1->m_occupied.contains(origin2));
		CHECK(fg1->m_occupied.volume() == 1);
		CHECK(fg1->m_flowingUp);
		CHECK(fg2->m_stable);
		CHECK(fg2->m_occupied.volume() == 2);
		// Step 3. CO2 flows up to above water at origin3.
		simulation.doStep();
		CHECK(!fg1->m_stable);
		CHECK(fg1->m_occupied.contains(origin3));
		CHECK(fg1->m_occupied.volume() == 1);
		// Step 4. Both stable.
		simulation.doStep();
		CHECK(fg1->m_stable);
		CHECK(!fg1->m_flowingUp);
		// Remove one point worth of fluid from water.
		fg2->removeFluid(area, maxPointVolume);
		CHECK(!fg2->m_stable);
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin3, CO2) == maxPointVolume);
		CHECK(!fg1->m_stable);
		// Step 5.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == maxPointVolume);
		CHECK(fg2->m_stable);
		simulation.doStep();
		CHECK(fg1->m_stable);
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
		space.fluid_add(origin1.toSet(), maxPointVolume, CO2);
		space.fluid_add(origin2.toSet(), maxPointVolume, water);
		space.fluid_add(origin3.toSet(), maxPointVolume, mercury);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, CO2);
		FluidGroup* fg2 = space.fluid_getGroup(origin2, water);
		FluidGroup* fg3 = space.fluid_getGroup(origin3, mercury);
		// Step 1. Water and mercury both sink down, water and CO2 are both at point1.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, CO2) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, mercury) == maxPointVolume);
		CHECK(!space.fluid_any(origin3));
		// Step 2. CO2 rises to origin2, mercury sinks to origin1, water is marked unstable.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin1, mercury) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, mercury) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == maxPointVolume);
		CHECK(!space.fluid_any(origin3));
		CHECK(!space.fluid_any(point1));
		fg1 = space.fluid_getGroup(origin2, CO2);
		fg2 = space.fluid_getGroup(origin1, water);
		// Step 3. mercury is marked stable, water rises to origin2. CO2 stays put.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, mercury) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin2, water) == maxPointVolume);
		CHECK(space.fluid_volumeOfTypeContains(origin2, CO2) == maxPointVolume);
		CHECK(space.fluid_getTotalVolume(point1) == 0);
		CHECK(space.fluid_getTotalVolume(origin3) == 0);
		// Step 5. water is stable, CO2 rises to origin3.
		simulation.doStep();
		CHECK(fg2->m_stable);
		CHECK(fg3->m_stable);
		CHECK(space.fluid_volumeOfTypeContains(origin3, CO2) == maxPointVolume);
		// Step 6. CO2 is stable.
		simulation.doStep();
		CHECK(fg1->m_stable);
		CHECK(fg2->m_stable);
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
		space.fluid_add(origin1.toSet(), maxPointVolume, water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		CHECK(fg1 != nullptr);
		// Step 1.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume);
		CHECK(fg1->m_stable);
		// Step 2.
		space.solid_setNot(point1);
		CHECK(!fg1->m_stable);
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume / 2);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == maxPointVolume / 2);
		// Step .
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume / 3);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == maxPointVolume / 3);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == maxPointVolume / 3);
		CHECK(!fg1->m_stable);
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
		space.fluid_add(origin1.toSet(), maxPointVolume, water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		// Step 1.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume / 2);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == maxPointVolume / 2);
		space.solid_set(point1, marble, false);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume);
		fg1 = space.fluid_getGroup(origin1, water);
		CHECK(fg1->m_volume == maxPointVolume);
		simulation.doStep();
		simulation.doStep();
		CHECK(fg1->m_stable);
	}
	SUBCASE("Set solid and split")
	{
		// Create a 3x1 trench and put maxPointVolume water in the middle point.
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
		space.fluid_add(origin1.toSet(), maxPointVolume, water);
		FluidGroup* fg1 = space.fluid_getGroup(origin1, water);
		// Step 1.
		simulation.doStep();
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == maxPointVolume / 3);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == maxPointVolume / 3);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == maxPointVolume / 3);
		space.solid_set(origin1, marble, false);
		CHECK(space.solid_isAny(origin1));
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == maxPointVolume / 2);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == maxPointVolume / 2);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 0);
		//Step 2.
		simulation.doStep();
		fg1 = space.fluid_getGroup(point1, water);
		FluidGroup* fg2 = space.fluid_getGroup(point2, water);
		CHECK(fg1 != fg2);
		simulation.doStep();
		assert(area.m_hasFluidGroups.m_groups.size() == 2);
		fg1 = space.fluid_getGroup(point1, water);
		fg2 = space.fluid_getGroup(point2, water);
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
		space.fluid_add(point1.toSet(), maxPointVolume, water);
		space.solid_set(point2, marble, false);
		support.maybeFall({point2, point2});
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		simulation.doStep();
		support.doStep(area);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(space.fluid_getTotalVolume(point1) == 0);
		CHECK(space.solid_get(point1) == marble);
		CHECK(!space.solid_isAny(point2));
		CHECK(fluidGroup->m_occupied.volume() == 0);
		CHECK(!space.solid_isAny(point2));
		simulation.doStep();
		support.doStep(area);
		CHECK(space.fluid_getTotalVolume(point2) == maxPointVolume);
		CHECK(!fluidGroup->m_stable);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
	}
}
TEST_CASE("fluidsLarger")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(20,20,20);
	area.m_hasRain.disable();
	Space& space = area.getSpace();
	int64_t maxPointVolume = Config::maxPointVolume.get();
	SUBCASE("Flow across flat area double stack")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D origin1 = Point3D::create(10, 10, 1);
		Point3D origin2 = Point3D::create(10, 10, 2);
		Point3D point1 = Point3D::create(16, 10, 1);
		space.fluid_add(Cuboid::create(origin1, origin2).toSet(), maxPointVolume * 2, water);
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_occupied.volume() == 2);
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_occupied.volume() == 9);
		CHECK(!fluidGroup->m_stable);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 25);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 49);
		CHECK(!fluidGroup->m_stable);
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_occupied.volume() == 81);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 121);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 169);
		CHECK(space.fluid_volumeOfTypeContains(origin1, water) == 1);
		CHECK(space.fluid_volumeOfTypeContains(point1, water) == 1);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 169);
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 169);
		CHECK(fluidGroup->m_stable);
	}
	SUBCASE("Flow across flat area")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Point3D point = Point3D::create(10, 10, 1);
		Point3D point2 = Point3D::create(10, 12, 1);
		space.fluid_add(point.toSet(), Config::maxPointVolume.get(), water);
		FluidGroup* fluidGroup = &area.m_hasFluidGroups.m_groups[0];
		//Step 1.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 9);
		CHECK(space.fluid_volumeOfTypeContains(point, water) == 11);
		CHECK(space.fluid_volumeOfTypeContains(point2, water) == 0);
		for(const Point3D& adjacent : space.getAdjacentWithEdgeAndCornerAdjacent(point))
			if(adjacent.z() == 1)
				CHECK(space.fluid_volumeOfTypeContains(adjacent, water) == 11);
		CHECK(!fluidGroup->m_stable);
		//Step 2.
		area.m_hasFluidGroups.doStep();
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		CHECK(fluidGroup->m_occupied.volume() == 25);
		//Step 3.
		area.m_hasFluidGroups.doStep();
		CHECK(!fluidGroup->m_stable);
		//Step 4.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 81);
		//Step 5.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 81);
		//Step 6.
		area.m_hasFluidGroups.doStep();
		CHECK(fluidGroup->m_occupied.volume() == 81);
		CHECK(fluidGroup->m_stable);
	}
}
TEST_CASE("fluidsMultiScale")
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
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
		CHECK(!fgWater->m_merged);
		CHECK(!fgCO2->m_merged);
		int totalVolume = fgWater->m_volume;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep();
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = 1 + (maxZ - 1) / 2;
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(fgWater->m_stable);
		CHECK(fgWater->m_occupied.volume() == expectedBlocks);
		CHECK(fgWater->m_volume == totalVolume);
		CHECK(fgCO2->m_stable);
		CHECK(fgCO2->m_occupied.volume() == expectedBlocks);
		CHECK(fgCO2->m_volume == totalVolume);
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
		trenchTest2Fluids(40, 1, Step::create(22));
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
		CHECK(area.m_hasFluidGroups.m_groups.size() == 3);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = space.fluid_getGroup(lava1, lava);
		simulation.m_step = Step::create(1);
		int totalVolume = fgWater->m_volume;
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep();
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = std::max(1, maxZ / 3);
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(fgWater->m_stable);
		CHECK(fgWater->m_occupied.volume() == expectedBlocks);
		CHECK(fgWater->m_volume == totalVolume);
		CHECK(fgCO2->m_stable);
		CHECK(fgCO2->m_occupied.volume() == expectedBlocks);
		CHECK(fgCO2->m_volume == totalVolume);
		CHECK(fgLava->m_stable);
		CHECK(fgLava->m_occupied.volume() == expectedBlocks);
		CHECK(fgLava->m_volume == totalVolume);
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
		trenchTest3Fluids(18, 3, Step::create(40));
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
		CHECK(area.m_hasFluidGroups.m_groups.size() == 4);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = space.fluid_getGroup(lava1, lava);
		FluidGroup* fgMercury = space.fluid_getGroup(mercury1, mercury);
		int totalVolume = fgWater->m_volume;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep();
			space.prepareRtrees();
			fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
			if(fgMercury != nullptr)
				CHECK(fgMercury->m_volume == totalVolume);
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
		CHECK(fgWater->m_occupied.volume() == expectedBlocks);
		CHECK(fgWater->m_volume == totalVolume);
		CHECK(fgCO2->m_stable);
		CHECK(fgCO2->m_occupied.volume() == expectedBlocks);
		CHECK(fgCO2->m_volume == totalVolume);
		CHECK(fgLava->m_stable);
		CHECK(fgLava->m_occupied.volume() == expectedBlocks);
		CHECK(fgLava->m_volume == totalVolume);
		CHECK(fgMercury->m_stable);
		CHECK(fgMercury->m_occupied.volume() == expectedBlocks);
		CHECK(fgMercury->m_volume == totalVolume);
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, 1), mercury));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, 1), mercury));
		CHECK(space.fluid_getGroup(Point3D::create(1, 1, maxZ - 1), CO2));
		CHECK(space.fluid_getGroup(Point3D::create(maxX - 2, 1, maxZ - 1), CO2));
		CHECK(area.m_hasFluidGroups.m_groups.size() == 4);
		CHECK(!area.m_hasFluidGroups.hasUnstable());
	};
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
		trenchTest4Fluids(16, 4, Step::create(28));
	}
	SUBCASE("trench test 4 fluids scale 16-16")
	{
		trenchTest4Fluids(16, 16, Step::create(28));
	}
	SUBCASE("trench test 4 fluids scale 40-40")
	{
		trenchTest4Fluids(40, 40, Step::create(48));
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
		CHECK(area.m_hasFluidGroups.m_groups.size() == 4);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		int totalVolume = fgWater->m_volume * 2u;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep();
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = std::max(1, maxZ / 2);
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(!area.m_hasFluidGroups.hasUnstable());
		fgWater = space.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = space.fluid_getGroup(water2, CO2);
		CHECK(fgWater->m_volume == totalVolume);
		CHECK(fgCO2->m_volume == totalVolume);
		CHECK(fgWater->m_occupied.volume() == expectedBlocks);
		CHECK(fgCO2->m_occupied.volume() == expectedBlocks);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 2);
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
		CHECK(area.m_hasFluidGroups.m_groups.size() == 4);
		FluidGroup* fgWater = space.fluid_getGroup(water1, water);
		int totalVolumeWater = fgWater->m_volume;
		int totalVolumeMercury = totalVolumeWater;
		int totalVolumeCO2 = totalVolumeWater * 2;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep();
			space.prepareRtrees();
			++simulation.m_step;
		}
		int totalBlocks2D = (maxX - 2) * (maxY - 2);
		int expectedHeight = std::max(1, maxZ / 4);
		int expectedBlocks = totalBlocks2D * expectedHeight;
		CHECK(!area.m_hasFluidGroups.hasUnstable());
		fgWater = areaBuilderUtil::getFluidGroup(area, water);
		FluidGroup* fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
		FluidGroup* fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
		CHECK(fgWater != nullptr);
		CHECK(fgCO2 != nullptr);
		CHECK(fgMercury != nullptr);
		CHECK(fgWater->m_volume == totalVolumeWater);
		CHECK(fgCO2->m_volume == totalVolumeCO2);
		CHECK(fgMercury->m_volume == totalVolumeMercury);
		CHECK(fgWater->m_occupied.volume() == expectedBlocks);
		CHECK((fgCO2->m_occupied.volume() == expectedBlocks || fgCO2->m_occupied.volume() == expectedBlocks * 2));
		CHECK(fgMercury->m_occupied.volume() == expectedBlocks);
		CHECK(fgWater->m_stable);
		CHECK(fgCO2->m_stable);
		CHECK(fgMercury->m_stable);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 3);
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