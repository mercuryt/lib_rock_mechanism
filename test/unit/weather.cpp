#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/threadedTask.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "../../engine/config/config.h"
#include <iterator>
#include <iostream>

TEST_CASE("weather")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static MaterialTypeId ice = MaterialType::byName("ice");
	static FluidTypeId water = FluidType::byName("water");
	const Temperature& freezing = FluidType::getFreezingPoint(water);
	Simulation simulation{"", Step::create(1)};
	SUBCASE("rain")
	{
		Area& area = simulation.m_hasAreas->createArea(5, 5, 5);
		Space& space = area.getSpace();
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Step duration = Config::stepsPerMinute * 2;
		area.m_hasRain.start(Percent::create(100), duration);
		CHECK(area.m_hasRain.isRaining());
		for(int i = 0; i < duration; ++i)
			simulation.doStep();
		simulation.doStep();
		CHECK(!area.m_hasRain.isRaining());
		CHECK(space.fluid_volumeOfTypeContains(Point3D::create(2,2,1), water) != 0);
	}
	SUBCASE("exposed to sky")
	{
		Area& area = simulation.m_hasAreas->createArea(1, 1, 3);
		Space& space = area.getSpace();
		const Point3D& top = Point3D::create(0, 0, 2);
		const Point3D& mid = Point3D::create(0, 0, 1);
		const Point3D& bot = Point3D::create(0, 0, 0);
		CHECK(space.isExposedToSky(top));
		CHECK(space.isExposedToSky(mid));
		CHECK(space.isExposedToSky(bot));
		space.solid_set(mid, marble, false);
		CHECK(space.isExposedToSky(top));
		CHECK(space.isExposedToSky(mid));
		CHECK(!space.isExposedToSky(bot));
		space.solid_set(top, marble, false);
		CHECK(space.isExposedToSky(top));
		CHECK(!space.isExposedToSky(mid));
		CHECK(!space.isExposedToSky(bot));
		space.solid_setNot(mid);
		CHECK(!space.isExposedToSky(mid));
		space.solid_setNot(top);
		CHECK(space.isExposedToSky(bot));
		CHECK(space.isExposedToSky(mid));
		CHECK(space.isExposedToSky(top));
	}
	SUBCASE("freeze and thaw")
	{
		Area& area = simulation.m_hasAreas->createArea(5, 5, 5);
		Space& space = area.getSpace();
		Items& items = area.getItems();
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		const Point3D& above = Point3D::create(2, 2, 4);
		CHECK(space.isExposedToSky(above));
		const Point3D& pond1 = Point3D::create(2, 2, 1);
		const Point3D& pond2 = Point3D::create(2, 2, 2);
		const Point3D& pond3 = Point3D::create(2, 2, 3);
		space.solid_setNot(pond1);
		CHECK(!space.isExposedToSky(pond1));
		space.solid_setNot(pond2);
		CHECK(!space.isExposedToSky(pond2));
		space.solid_setNot(pond3);
		CHECK(space.isExposedToSky(pond1));
		CHECK(space.isExposedToSky(pond2));
		CHECK(space.isExposedToSky(pond3));
		space.fluid_add(Cuboid::create(pond1, pond3).toSet(), 299, water);
		auto& hasTemperature = area.m_hasTemperature;
		CHECK(hasTemperature.m_freezableFluidTypeOnSurface.contains(water));
		CHECK(!hasTemperature.m_freezableFluidTypeOnSurface[water].empty());
		CHECK(!hasTemperature.m_meltableMaterialTypeOnSurface.contains(ice));
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		hasTemperature.setAmbient(area, freezing - 1);
		CHECK(area.m_hasFluidGroups.m_groups.size() == 1);
		// Point 3 is not full so it turns into chunks.
		CHECK(!space.solid_isAny(pond3));
		static const ItemTypeId& chunk = ItemType::byName("chunk");
		CHECK(space.item_getCount(pond3, chunk, ice) == Quantity::create(33));
		// Point 1 is 2 depth from the surface and won't ever freeze.
		CHECK(!space.solid_isAny(pond1));
		CHECK(space.solid_isAny(pond2));
		CHECK(space.item_empty(above));
		CHECK(hasTemperature.m_freezableFluidTypeOnSurface[water].empty());
		CHECK(hasTemperature.m_meltableMaterialTypeOnSurface.contains(ice));
		auto& blocksByMaterialType = hasTemperature.m_meltableMaterialTypeOnSurface[ice];
		CHECK(blocksByMaterialType.solid.size() == 1);
		const ItemIndex& chunk1 = space.item_getGeneric(pond3, chunk, ice);
		CHECK(items.getQuantity(chunk1) == Quantity::create(33));
		CHECK(items.isOnSurface(chunk1));
		CHECK(area.m_hasTemperature.m_meltableMaterialTypeOnSurface[ice].items.contains(pond3));
		CHECK(space.fluid_volumeOfTypeContains(pond1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(pond2, water) == 0);
		CHECK(space.fluid_volumeOfTypeContains(pond3, water) == 0);
		hasTemperature.setAmbient(area, freezing + 1);
		CHECK(!space.solid_isAny(pond3));
		CHECK(space.item_empty(pond3));
		CHECK(!space.solid_isAny(pond2));
		CHECK(!space.solid_isAny(pond1));
		CHECK(!hasTemperature.m_freezableFluidTypeOnSurface[water].empty());
		CHECK(hasTemperature.m_meltableMaterialTypeOnSurface[ice].empty());
		CHECK(space.fluid_volumeOfTypeContains(pond1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(pond2, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(pond3, water) == 99);
		CHECK(space.fluid_getTotalVolume(above) == 0);
	}
	SUBCASE("ambient temperature and exterior portals")
	{
		Area& area = simulation.m_hasAreas->createArea(10, 10, 5);
		Space& space = area.getSpace();
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		const Point3D& above = Point3D::create(5, 0, 3);
		const Point3D& outside = Point3D::create(5, 0, 2);
		const Point3D& portal = Point3D::create(5, 1, 2);
		const Point3D& block1 = Point3D::create(5, 2, 2);
		space.solid_setNot(above);
		space.solid_setNot(outside);
		space.solid_setNot(portal);
		// isPortal only works if there is at least one block to be affected by the portal.
		CHECK(!area.m_hasTemperature.m_portals.isPortal(area, portal));
		space.solid_setNot(block1);
		CHECK(area.m_hasTemperature.m_portals.isPortal(area, portal));
		CHECK(area.m_hasTemperature.m_portals.isRecordedAsPortal(portal));
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block1) == 1);
		const Point3D& block2 = Point3D::create(5, 3, 2);
		const Point3D& block3 = Point3D::create(5, 4, 2);
		// block2 is affected despite being solid.
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block2) == 2);
		space.solid_setNot(block2);
		// Block 3 is blocked by block2.
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block3).empty());
		CHECK(area.m_hasTemperature.m_portals.getToUpdateSize() == 1);
		area.m_hasTemperature.m_portals.doStep(area);
		CHECK(area.m_hasTemperature.m_portals.getToUpdateSize() == 0);
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block2) == 2);
		// block3 is affected after update.
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block3) == 3);
		space.solid_setNot(block3);
		area.m_hasTemperature.m_portals.doStep(area);
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block3) == 3);
		const Point3D& block4 = Point3D::create(5, 5, 2);
		space.solid_setNotCuboid({Point3D::create(5, 9, 2), block4});
		area.m_hasTemperature.m_portals.doStep(area);
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block4) == 4);
		// Set ambiant below interior / underground so we know which way the gradiant is suposed to go.
		area.m_hasTemperature.setAmbient(area, freezing - 10);
		CHECK(space.temperature_get(outside) == space.temperature_get(portal));
		CHECK(space.temperature_get(portal) < space.temperature_get(block1));
		CHECK(space.temperature_get(block1) < space.temperature_get(block2));
		CHECK(space.temperature_get(block2) < space.temperature_get(block3));
		CHECK(space.temperature_get(block3) < space.temperature_get(block4));
		// Remove roof over portal.
		space.solid_setNot(Point3D::create(5, 1, 3));
		CHECK(space.isExposedToSky(portal));
		CHECK(!space.isExposedToSky(block1));
		CHECK(!area.m_hasTemperature.m_portals.isRecordedAsPortal(portal));
		CHECK(area.m_hasTemperature.m_portals.isRecordedAsPortal(block1));
		CHECK(space.temperature_get(block1) < space.temperature_get(block2));
		CHECK(space.temperature_get(block2) < space.temperature_get(block3));
		CHECK(space.temperature_get(block3) < space.temperature_get(block4));
		const Point3D& block5 = Point3D::create(5, 8, 2);
		CHECK(space.temperature_get(block4) < space.temperature_get(block5));
		const Point3D& block6 = Point3D::create(5, 9, 2);
		CHECK(space.temperature_get(block5) < space.temperature_get(block6));
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block5).exists());
		CHECK(area.m_hasTemperature.m_portals.queryDistanceToNearest(block6).empty());
		space.pointFeature_construct(block1, PointFeatureTypeId::Door, MaterialType::byName("poplar wood"));
		CHECK(!area.m_hasTemperature.m_portals.isRecordedAsPortal(block1));
		CHECK(!area.m_hasTemperature.m_portals.isRecordedAsPortal(portal));
		CHECK(space.temperature_get(block2) == space.temperature_get(block4));
	}
}
