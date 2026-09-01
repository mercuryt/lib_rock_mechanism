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
		static const ItemTypeId& chunk = ItemType::byName("chunk");
		static const ItemTypeId& pile = ItemType::byName("pile");
		Area& area = simulation.m_hasAreas->createArea(5, 5, 5);
		Space& space = area.getSpace();
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
		int chunksPerSolid = Config::maxPointVolume.get() / Shape::getTotalCollisionVolume(ItemType::getShape(chunk)).get();
		// One unit freezes into a pile.
		space.fluid_add(pond1.toSet(), 1, water);
		auto& hasTemperature = area.m_hasTemperature;
		hasTemperature.m_maxAmbiant = hasTemperature.m_minAmbiant = freezing - 1;
		hasTemperature.setAmbient(area, freezing - 1);
		CHECK(hasTemperature.m_freezableFluidTypeOnSurface.contains(water));
		CHECK(!hasTemperature.m_freezableFluidTypeOnSurface[water].empty());
		CHECK(!hasTemperature.m_meltableMaterialTypeOnSurface.contains(ice));
		CHECK(area.m_hasPhaseChanges.m_freezing.contains(water));
		area.m_hasPhaseChanges.doFreeze(area, water, pond1.toSet());
		CHECK(space.item_getCount(pond1, pile, ice) == 1);
		// Three piles freeze into a chunk.
		space.fluid_add(pond1.toSet(), 2, water);
		area.m_hasPhaseChanges.doFreeze(area, water, pond1.toSet());
		CHECK(space.item_getCount(pond1, pile, ice) == 0);
		CHECK(space.item_getCount(pond1, chunk, ice) == 1);
		// chunksPerSolid number of chunks plus one water freezes solid (this depends on the volume of chunk being 3 and maxpointvolume being 100).
		space.item_addGeneric(pond1, chunk, ice, {chunksPerSolid - 1});
		space.fluid_add(pond1.toSet(), 1, water);
		area.m_hasPhaseChanges.doFreeze(area, water, pond1.toSet());
		CHECK(space.solid_isAny(pond1));
		CHECK(!space.fluid_any(space.boundry()));
		space.item_addGeneric(pond2, chunk, ice, {chunksPerSolid});
		space.fluid_add(pond2.toSet(), 2, water);
		area.m_hasPhaseChanges.doFreeze(area, water, pond2.toSet());
		CHECK(space.solid_isAny(pond2));
		// Overfull ejects extra.
		CHECK(space.fluid_volumeOfTypeContains(pond3, water) == 1);
		area.m_hasPhaseChanges.doFreeze(area, water, pond3.toSet());
		CHECK(!space.fluid_any(space.boundry()));
		CHECK(space.item_getCount(pond3, pile, ice) == 1);
		// Melting.
		hasTemperature.m_maxAmbiant = hasTemperature.m_minAmbiant = freezing + 1;
		hasTemperature.setAmbient(area, freezing + 1);
		CHECK(area.m_hasPhaseChanges.m_melting.contains(ice));
		CHECK(area.m_hasPhaseChanges.m_melting[ice].contains(pond3));
		CHECK(area.m_hasPhaseChanges.m_melting[ice].contains(pond2));
		// Only the top solid layer melts. The others will have to wait.
		CHECK(!area.m_hasPhaseChanges.m_melting[ice].contains(pond1));
		area.m_hasPhaseChanges.doMelt(area, ice, pond3.toSet());
		CHECK(!area.m_hasPhaseChanges.m_melting[ice].contains(pond3));
		CHECK(space.fluid_volumeOfTypeContains(pond3, water) == 1);
		CHECK(space.item_empty(pond3));
		area.m_hasPhaseChanges.doMelt(area, ice, pond2.toSet());
		CHECK(space.fluid_volumeOfTypeContains(pond3, water) == 1);
		CollisionVolume chunkDisplacement = Shape::getTotalCollisionVolume(ItemType::getShape(chunk));
		int chunkCount = ((Config::maxPointVolume - 1) / chunkDisplacement).get();
		CHECK(space.item_getCount(pond2, chunk, ice) == chunkCount);
		CHECK(space.item_getCount(pond2, pile, ice) == Config::maxPointVolume.get() - 1 - chunkCount * chunkDisplacement.get());
		CHECK(area.m_hasPhaseChanges.m_melting[ice].contains(pond2));
		CHECK(!area.m_hasPhaseChanges.m_melting[ice].contains(pond3));
		// TODO: test melting features.
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
