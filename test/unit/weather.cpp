#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/threadedTask.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "config.h"
#include <iterator>
#include <iostream>

TEST_CASE("weather")
{
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static MaterialTypeId ice = MaterialType::byName(L"ice");
	static FluidTypeId water = FluidType::byName(L"water");
	const Temperature& freezing = FluidType::getFreezingPoint(water);
	Simulation simulation{L"", Step::create(1)};
	SUBCASE("rain")
	{
		Area& area = simulation.m_hasAreas->createArea(5, 5, 5);
		Blocks& blocks = area.getBlocks();
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Step duration = Config::stepsPerMinute * 2;
		area.m_hasRain.start(Percent::create(100), duration);
		REQUIRE(area.m_hasRain.isRaining());
		for(uint i = 0; i < duration; ++i)
			simulation.doStep();
		simulation.doStep();
		REQUIRE(!area.m_hasRain.isRaining());
		REQUIRE(blocks.fluid_volumeOfTypeContains(blocks.getIndex_i(2,2,1), water) != 0);
	}
	SUBCASE("exposed to sky")
	{
		Area& area = simulation.m_hasAreas->createArea(1, 1, 3);
		Blocks& blocks = area.getBlocks();
		const BlockIndex& top = blocks.getIndex_i(0, 0, 2);
		const BlockIndex& mid = blocks.getIndex_i(0, 0, 1);
		const BlockIndex& bot = blocks.getIndex_i(0, 0, 0);
		REQUIRE(blocks.isExposedToSky(top));
		REQUIRE(blocks.isExposedToSky(mid));
		REQUIRE(blocks.isExposedToSky(bot));
		blocks.solid_set(mid, marble, false);
		REQUIRE(blocks.isExposedToSky(top));
		REQUIRE(blocks.isExposedToSky(mid));
		REQUIRE(!blocks.isExposedToSky(bot));
		blocks.solid_set(top, marble, false);
		REQUIRE(blocks.isExposedToSky(top));
		REQUIRE(!blocks.isExposedToSky(mid));
		REQUIRE(!blocks.isExposedToSky(bot));
		blocks.solid_setNot(mid);
		REQUIRE(!blocks.isExposedToSky(mid));
		blocks.solid_setNot(top);
		REQUIRE(blocks.isExposedToSky(bot));
		REQUIRE(blocks.isExposedToSky(mid));
		REQUIRE(blocks.isExposedToSky(top));
	}
	SUBCASE("freeze and thaw")
	{
		Area& area = simulation.m_hasAreas->createArea(5, 5, 5);
		Blocks& blocks = area.getBlocks();
		areaBuilderUtil::setSolidLayers(area, 0, 3, marble);
		const BlockIndex& above = blocks.getIndex_i(2, 2, 4);
		REQUIRE(blocks.isExposedToSky(above));
		const BlockIndex& pond1 = blocks.getIndex_i(2, 2, 1);
		const BlockIndex& pond2 = blocks.getIndex_i(2, 2, 2);
		const BlockIndex& pond3 = blocks.getIndex_i(2, 2, 3);
		blocks.solid_setNot(pond1);
		REQUIRE(!blocks.isExposedToSky(pond1));
		blocks.solid_setNot(pond2);
		REQUIRE(!blocks.isExposedToSky(pond2));
		blocks.solid_setNot(pond3);
		REQUIRE(blocks.isExposedToSky(pond1));
		REQUIRE(blocks.isExposedToSky(pond2));
		REQUIRE(blocks.isExposedToSky(pond3));
		const Cuboid pond{blocks, pond3, pond1};
		blocks.fluid_add(pond, CollisionVolume::create(100), water);
		auto& hasTemperature = area.m_hasTemperature;
		REQUIRE(hasTemperature.getAboveGroundFluidGroupsByMeltingPoint().contains(freezing));
		REQUIRE(!hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		REQUIRE(hasTemperature.getAboveGroundBlocksByMeltingPoint()[freezing].empty());
		hasTemperature.setAmbientSurfaceTemperature(freezing - 1);
		REQUIRE(blocks.solid_is(pond3));
		REQUIRE(blocks.solid_get(pond3) == ice);
		REQUIRE(blocks.solid_is(pond1));
		REQUIRE(blocks.solid_is(pond2));
		REQUIRE(hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		REQUIRE(hasTemperature.getAboveGroundBlocksByMeltingPoint().contains(freezing));
		auto& blocksByMeltingPoint = hasTemperature.getAboveGroundBlocksByMeltingPoint()[freezing];
		REQUIRE(blocksByMeltingPoint.contains(pond3));
		REQUIRE(blocksByMeltingPoint.size() == 1);
		hasTemperature.setAmbientSurfaceTemperature(freezing + 1);
		REQUIRE(!blocks.solid_is(pond3));
		REQUIRE(!blocks.solid_is(pond2));
		REQUIRE(!blocks.solid_is(pond1));
		REQUIRE(!hasTemperature.getAboveGroundFluidGroupsByMeltingPoint()[freezing].empty());
		REQUIRE(hasTemperature.getAboveGroundBlocksByMeltingPoint()[freezing].empty());
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
		REQUIRE(AreaHasExteriorPortals::isPortal(blocks, portal));
		REQUIRE(area.m_exteriorPortals.isRecordedAsPortal(portal));
		const BlockIndex& block1 = blocks.getIndex_i(5, 2, 2);
		blocks.solid_setNot(block1);
		REQUIRE(area.m_exteriorPortals.getDistanceFor(block1) == 1);
		const BlockIndex& block2 = blocks.getIndex_i(5, 3, 2);
		blocks.solid_setNot(block2);
		REQUIRE(area.m_exteriorPortals.getDistanceFor(block2) == 2);
		const BlockIndex& block3 = blocks.getIndex_i(5, 4, 2);
		blocks.solid_setNot(block3);
		REQUIRE(area.m_exteriorPortals.getDistanceFor(block3) == 3);
		const BlockIndex& block4 = blocks.getIndex_i(5, 5, 2);
		blocks.solid_setNotCuboid({Point3D::create(5, 9, 2), blocks.getCoordinates(block4)});
		REQUIRE(area.m_exteriorPortals.getDistanceFor(block4) == 4);
		// Set ambiant below interior / underground so we know which way the gradiant is suposed to go.
		area.m_hasTemperature.setAmbientSurfaceTemperature(freezing - 10);
		REQUIRE(blocks.temperature_get(outside) <= blocks.temperature_get(portal));
		REQUIRE(blocks.temperature_get(portal) < blocks.temperature_get(block1));
		REQUIRE(blocks.temperature_get(block1) < blocks.temperature_get(block2));
		REQUIRE(blocks.temperature_get(block2) < blocks.temperature_get(block3));
		REQUIRE(blocks.temperature_get(block3) < blocks.temperature_get(block4));
		// Remove roof over portal.
		blocks.solid_setNot(blocks.getIndex_i(5, 1, 3));
		REQUIRE(blocks.isExposedToSky(portal));
		REQUIRE(!blocks.isExposedToSky(block1));
		REQUIRE(!area.m_exteriorPortals.isRecordedAsPortal(portal));
		REQUIRE(area.m_exteriorPortals.isRecordedAsPortal(block1));
		REQUIRE(blocks.temperature_get(block1) < blocks.temperature_get(block2));
		REQUIRE(blocks.temperature_get(block2) < blocks.temperature_get(block3));
		REQUIRE(blocks.temperature_get(block3) < blocks.temperature_get(block4));
		const BlockIndex& lastBlockInPortalEffectZone = blocks.getIndex_i(5, Config::maxDepthExteriorPortalPenetration.get() + 1, 2);
		const BlockIndex& firstBlockAfterEffectZone = blocks.getIndex_i(5, Config::maxDepthExteriorPortalPenetration.get() + 2, 2);
		REQUIRE(blocks.temperature_get(block4) < blocks.temperature_get(lastBlockInPortalEffectZone));
		REQUIRE(blocks.temperature_get(lastBlockInPortalEffectZone) < blocks.temperature_get(firstBlockAfterEffectZone));
		REQUIRE(area.m_exteriorPortals.getDistanceFor(lastBlockInPortalEffectZone).exists());
		REQUIRE(area.m_exteriorPortals.getDistanceFor(firstBlockAfterEffectZone).empty());
		blocks.blockFeature_construct(block1, BlockFeatureType::door, MaterialType::byName(L"poplar wood"));
		REQUIRE(!area.m_exteriorPortals.isRecordedAsPortal(block1));
		REQUIRE(!area.m_exteriorPortals.isRecordedAsPortal(portal));
		REQUIRE(blocks.temperature_get(block2) == blocks.temperature_get(block4));
	}
}
