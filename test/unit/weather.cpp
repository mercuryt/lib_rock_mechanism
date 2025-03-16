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
		Blocks& blocks = area.getBlocks();
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Step duration = Config::stepsPerMinute * 2;
		area.m_hasRain.start(Percent::create(100), duration);
		CHECK(area.m_hasRain.isRaining());
		for(uint i = 0; i < duration; ++i)
			simulation.doStep();
		simulation.doStep();
		CHECK(!area.m_hasRain.isRaining());
		CHECK(blocks.fluid_volumeOfTypeContains(blocks.getIndex_i(2,2,1), water) != 0);
	}
	SUBCASE("exposed to sky")
	{
		Area& area = simulation.m_hasAreas->createArea(1, 1, 3);
		Blocks& blocks = area.getBlocks();
		const BlockIndex& top = blocks.getIndex_i(0, 0, 2);
		const BlockIndex& mid = blocks.getIndex_i(0, 0, 1);
		const BlockIndex& bot = blocks.getIndex_i(0, 0, 0);
		CHECK(blocks.isExposedToSky(top));
		CHECK(blocks.isExposedToSky(mid));
		CHECK(blocks.isExposedToSky(bot));
		blocks.solid_set(mid, marble, false);
		CHECK(blocks.isExposedToSky(top));
		CHECK(blocks.isExposedToSky(mid));
		CHECK(!blocks.isExposedToSky(bot));
		blocks.solid_set(top, marble, false);
		CHECK(blocks.isExposedToSky(top));
		CHECK(!blocks.isExposedToSky(mid));
		CHECK(!blocks.isExposedToSky(bot));
		blocks.solid_setNot(mid);
		CHECK(!blocks.isExposedToSky(mid));
		blocks.solid_setNot(top);
		CHECK(blocks.isExposedToSky(bot));
		CHECK(blocks.isExposedToSky(mid));
		CHECK(blocks.isExposedToSky(top));
	}
	SUBCASE("freeze and thaw")
	{
		Area& area = simulation.m_hasAreas->createArea(5, 5, 5);
		Blocks& blocks = area.getBlocks();
		Items& items = area.getItems();
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		const BlockIndex& above = blocks.getIndex_i(2, 2, 4);
		CHECK(blocks.isExposedToSky(above));
		const BlockIndex& pond1 = blocks.getIndex_i(2, 2, 1);
		const BlockIndex& pond2 = blocks.getIndex_i(2, 2, 2);
		const BlockIndex& pond3 = blocks.getIndex_i(2, 2, 3);
		blocks.solid_setNot(pond1);
		CHECK(!blocks.isExposedToSky(pond1));
		blocks.solid_setNot(pond2);
		CHECK(!blocks.isExposedToSky(pond2));
		blocks.solid_setNot(pond3);
		CHECK(blocks.isExposedToSky(pond1));
		CHECK(blocks.isExposedToSky(pond2));
		CHECK(blocks.isExposedToSky(pond3));
		blocks.fluid_add(pond1, CollisionVolume::create(100), water);
		blocks.fluid_add(pond2, CollisionVolume::create(100), water);
		blocks.fluid_add(pond3, CollisionVolume::create(99), water);
		auto& hasTemperature = area.m_hasTemperature;
		CHECK(hasTemperature.getAboveGroundFluidGroupsByMeltingPoint().contains(freezing));
		CHECK(!hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		CHECK(hasTemperature.getAboveGroundBlocksByMeltingPoint()[freezing].empty());
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		hasTemperature.setAmbientSurfaceTemperature(freezing - 1);
		CHECK(area.m_hasFluidGroups.getAll().empty());
		CHECK(!blocks.solid_is(pond3));
		static const ItemTypeId& chunk = ItemType::byName("chunk");
		CHECK(blocks.item_getCount(pond3, chunk, ice) == Quantity::create(33));
		CHECK(blocks.solid_is(pond1));
		CHECK(blocks.solid_is(pond2));
		CHECK(blocks.item_empty(above));
		CHECK(hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		CHECK(hasTemperature.getAboveGroundBlocksByMeltingPoint().contains(freezing));
		auto& blocksByMeltingPoint = hasTemperature.getAboveGroundBlocksByMeltingPoint()[freezing];
		CHECK(blocksByMeltingPoint.size() == 1);
		const ItemIndex& chunk1 = blocks.item_getGeneric(pond3, chunk, ice);
		CHECK(items.getQuantity(chunk1) == Quantity::create(33));
		CHECK(items.isOnSurface(chunk1));
		hasTemperature.setAmbientSurfaceTemperature(freezing + 1);
		CHECK(area.m_hasFluidGroups.getAll().size() == 1);
		CHECK(!blocks.solid_is(pond3));
		CHECK(blocks.item_empty(pond3));
		CHECK(!blocks.solid_is(pond2));
		CHECK(!blocks.solid_is(pond1));
		CHECK(!hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		CHECK(hasTemperature.getAboveGroundBlocksByMeltingPoint()[freezing].empty());
		CHECK(blocks.fluid_volumeOfTypeContains(pond1, water) == 100);
		CHECK(blocks.fluid_volumeOfTypeContains(pond2, water) == 100);
		CHECK(blocks.fluid_volumeOfTypeContains(pond3, water) == 99);
		CHECK(blocks.fluid_getGroup(pond1, water)->m_excessVolume == 0);
		CHECK(blocks.fluid_getTotalVolume(above) == 0);
	}
	SUBCASE("ambient temperature and exterior portals")
	{
		Area& area = simulation.m_hasAreas->createArea(10, 10, 5);
		Blocks& blocks = area.getBlocks();
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		const BlockIndex& above = blocks.getIndex_i(5, 0, 3);
		const BlockIndex& outside = blocks.getIndex_i(5, 0, 2);
		const BlockIndex& portal = blocks.getIndex_i(5, 1, 2);
		blocks.solid_setNot(above);
		blocks.solid_setNot(outside);
		blocks.solid_setNot(portal);
		CHECK(AreaHasExteriorPortals::isPortal(blocks, portal));
		CHECK(area.m_exteriorPortals.isRecordedAsPortal(portal));
		const BlockIndex& block1 = blocks.getIndex_i(5, 2, 2);
		blocks.solid_setNot(block1);
		CHECK(area.m_exteriorPortals.getDistanceFor(block1) == 1);
		const BlockIndex& block2 = blocks.getIndex_i(5, 3, 2);
		blocks.solid_setNot(block2);
		CHECK(area.m_exteriorPortals.getDistanceFor(block2) == 2);
		const BlockIndex& block3 = blocks.getIndex_i(5, 4, 2);
		blocks.solid_setNot(block3);
		CHECK(area.m_exteriorPortals.getDistanceFor(block3) == 3);
		const BlockIndex& block4 = blocks.getIndex_i(5, 5, 2);
		blocks.solid_setNotCuboid({Point3D::create(5, 9, 2), blocks.getCoordinates(block4)});
		CHECK(area.m_exteriorPortals.getDistanceFor(block4) == 4);
		// Set ambiant below interior / underground so we know which way the gradiant is suposed to go.
		area.m_hasTemperature.setAmbientSurfaceTemperature(freezing - 10);
		CHECK(blocks.temperature_get(outside) <= blocks.temperature_get(portal));
		CHECK(blocks.temperature_get(portal) < blocks.temperature_get(block1));
		CHECK(blocks.temperature_get(block1) < blocks.temperature_get(block2));
		CHECK(blocks.temperature_get(block2) < blocks.temperature_get(block3));
		CHECK(blocks.temperature_get(block3) < blocks.temperature_get(block4));
		// Remove roof over portal.
		blocks.solid_setNot(blocks.getIndex_i(5, 1, 3));
		CHECK(blocks.isExposedToSky(portal));
		CHECK(!blocks.isExposedToSky(block1));
		CHECK(!area.m_exteriorPortals.isRecordedAsPortal(portal));
		CHECK(area.m_exteriorPortals.isRecordedAsPortal(block1));
		CHECK(blocks.temperature_get(block1) < blocks.temperature_get(block2));
		CHECK(blocks.temperature_get(block2) < blocks.temperature_get(block3));
		CHECK(blocks.temperature_get(block3) < blocks.temperature_get(block4));
		const BlockIndex& lastBlockInPortalEffectZone = blocks.getIndex_i(5, Config::maxDepthExteriorPortalPenetration.get() + 1, 2);
		const BlockIndex& firstBlockAfterEffectZone = blocks.getIndex_i(5, Config::maxDepthExteriorPortalPenetration.get() + 2, 2);
		CHECK(blocks.temperature_get(block4) < blocks.temperature_get(lastBlockInPortalEffectZone));
		CHECK(blocks.temperature_get(lastBlockInPortalEffectZone) < blocks.temperature_get(firstBlockAfterEffectZone));
		CHECK(area.m_exteriorPortals.getDistanceFor(lastBlockInPortalEffectZone).exists());
		CHECK(area.m_exteriorPortals.getDistanceFor(firstBlockAfterEffectZone).empty());
		blocks.blockFeature_construct(block1, BlockFeatureType::door, MaterialType::byName("poplar wood"));
		CHECK(!area.m_exteriorPortals.isRecordedAsPortal(block1));
		CHECK(!area.m_exteriorPortals.isRecordedAsPortal(portal));
		CHECK(blocks.temperature_get(block2) == blocks.temperature_get(block4));
	}
}
