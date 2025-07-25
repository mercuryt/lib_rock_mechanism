#include "../../lib/doctest.h"
#include "../../engine/area/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/threadedTask.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/hasShapes.hpp"
#include "../../engine/plants.h"
#include "config.h"
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
		for(uint i = 0; i < duration; ++i)
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
		space.fluid_add(pond1, CollisionVolume::create(100), water);
		space.fluid_add(pond2, CollisionVolume::create(100), water);
		space.fluid_add(pond3, CollisionVolume::create(99), water);
		auto& hasTemperature = area.m_hasTemperature;
		CHECK(hasTemperature.getAboveGroundFluidGroupsByMeltingPoint().contains(freezing));
		CHECK(!hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		CHECK(hasTemperature.getAboveGroundPointsByMeltingPoint()[freezing].empty());
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		hasTemperature.setAmbientSurfaceTemperature(freezing - 1);
		CHECK(area.m_hasFluidGroups.getAll().empty());
		CHECK(!space.solid_is(pond3));
		static const ItemTypeId& chunk = ItemType::byName("chunk");
		CHECK(space.item_getCount(pond3, chunk, ice) == Quantity::create(33));
		CHECK(space.solid_is(pond1));
		CHECK(space.solid_is(pond2));
		CHECK(space.item_empty(above));
		CHECK(hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		CHECK(hasTemperature.getAboveGroundPointsByMeltingPoint().contains(freezing));
		auto& blocksByMeltingPoint = hasTemperature.getAboveGroundPointsByMeltingPoint()[freezing];
		CHECK(blocksByMeltingPoint.size() == 1);
		const ItemIndex& chunk1 = space.item_getGeneric(pond3, chunk, ice);
		CHECK(items.getQuantity(chunk1) == Quantity::create(33));
		CHECK(items.isOnSurface(chunk1));
		hasTemperature.setAmbientSurfaceTemperature(freezing + 1);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(!space.solid_is(pond3));
		CHECK(space.item_empty(pond3));
		CHECK(!space.solid_is(pond2));
		CHECK(!space.solid_is(pond1));
		CHECK(!hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		CHECK(hasTemperature.getAboveGroundPointsByMeltingPoint()[freezing].empty());
		CHECK(space.fluid_volumeOfTypeContains(pond1, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(pond2, water) == 100);
		CHECK(space.fluid_volumeOfTypeContains(pond3, water) == 99);
		CHECK(space.fluid_getGroup(pond1, water)->m_excessVolume == 0);
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
		space.solid_setNot(above);
		space.solid_setNot(outside);
		space.solid_setNot(portal);
		CHECK(AreaHasExteriorPortals::isPortal(space, portal));
		CHECK(area.m_exteriorPortals.isRecordedAsPortal(portal));
		const Point3D& block1 = Point3D::create(5, 2, 2);
		space.solid_setNot(block1);
		CHECK(area.m_exteriorPortals.getDistanceFor(block1) == 1);
		const Point3D& block2 = Point3D::create(5, 3, 2);
		space.solid_setNot(block2);
		CHECK(area.m_exteriorPortals.getDistanceFor(block2) == 2);
		const Point3D& block3 = Point3D::create(5, 4, 2);
		space.solid_setNot(block3);
		CHECK(area.m_exteriorPortals.getDistanceFor(block3) == 3);
		const Point3D& block4 = Point3D::create(5, 5, 2);
		space.solid_setNotCuboid({Point3D::create(5, 9, 2), block4});
		CHECK(area.m_exteriorPortals.getDistanceFor(block4) == 4);
		// Set ambiant below interior / underground so we know which way the gradiant is suposed to go.
		area.m_hasTemperature.setAmbientSurfaceTemperature(freezing - 10);
		CHECK(space.temperature_get(outside) <= space.temperature_get(portal));
		CHECK(space.temperature_get(portal) < space.temperature_get(block1));
		CHECK(space.temperature_get(block1) < space.temperature_get(block2));
		CHECK(space.temperature_get(block2) < space.temperature_get(block3));
		CHECK(space.temperature_get(block3) < space.temperature_get(block4));
		// Remove roof over portal.
		space.solid_setNot(Point3D::create(5, 1, 3));
		CHECK(space.isExposedToSky(portal));
		CHECK(!space.isExposedToSky(block1));
		CHECK(!area.m_exteriorPortals.isRecordedAsPortal(portal));
		CHECK(area.m_exteriorPortals.isRecordedAsPortal(block1));
		CHECK(space.temperature_get(block1) < space.temperature_get(block2));
		CHECK(space.temperature_get(block2) < space.temperature_get(block3));
		CHECK(space.temperature_get(block3) < space.temperature_get(block4));
		const Point3D& lastBlockInPortalEffectZone = Point3D::create(5, Config::maxDepthExteriorPortalPenetration.get() + 1, 2);
		const Point3D& firstBlockAfterEffectZone = Point3D::create(5, Config::maxDepthExteriorPortalPenetration.get() + 2, 2);
		CHECK(space.temperature_get(block4) < space.temperature_get(lastBlockInPortalEffectZone));
		CHECK(space.temperature_get(lastBlockInPortalEffectZone) < space.temperature_get(firstBlockAfterEffectZone));
		CHECK(area.m_exteriorPortals.getDistanceFor(lastBlockInPortalEffectZone).exists());
		CHECK(area.m_exteriorPortals.getDistanceFor(firstBlockAfterEffectZone).empty());
		space.pointFeature_construct(block1, PointFeatureTypeId::Door, MaterialType::byName("poplar wood"));
		CHECK(!area.m_exteriorPortals.isRecordedAsPortal(block1));
		CHECK(!area.m_exteriorPortals.isRecordedAsPortal(portal));
		CHECK(space.temperature_get(block2) == space.temperature_get(block4));
	}
}
