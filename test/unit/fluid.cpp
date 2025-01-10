#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/fluidGroup.h"
#include "../../engine/materialType.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/blocks/blocks.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "types.h"
TEST_CASE("fluids smaller")
{
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static FluidTypeId water = FluidType::byName(L"water");
	static FluidTypeId CO2 = FluidType::byName(L"CO2");
	static FluidTypeId mercury = FluidType::byName(L"mercury");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(10,10,10);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	SUBCASE("Create Fluid 100")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block = blocks.getIndex_i(5, 5, 1);
		blocks.solid_setNot(block);
		blocks.fluid_add(block, CollisionVolume::create(100), water);
		REQUIRE(blocks.fluid_contains(blocks.getIndex_i(5, 5, 1), water));
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->mergeStep(area);
		fluidGroup->splitStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 100);
		REQUIRE(!blocks.fluid_contains(blocks.getBlockSouth(block), water));
		REQUIRE(!blocks.fluid_contains(blocks.getBlockEast(block), water));
		REQUIRE(!blocks.fluid_contains(blocks.getBlockNorth(block), water));
		REQUIRE(!blocks.fluid_contains(blocks.getBlockWest(block), water));
		REQUIRE(!blocks.fluid_contains(blocks.getBlockBelow(block), water));
		REQUIRE(!blocks.fluid_contains(blocks.getBlockAbove(block), water));
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
	}
	/*
	SUBCASE("Create Fluid 2")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 4);
		blocks.solid_setNot(block);
		blocks.fluid_add(block, CollisionVolume::create(1), water);
		blocks.fluid_add(block2, CollisionVolume::create(1), water);
		REQUIRE(blocks.fluid_getGroup(block, &water));
		simulation.doStep();
		simulation.doStep();
		REQUIRE(blocks.fluid_getGroup(block, &water));
		REQUIRE(area.m_hasFluidGroups.getUnstable().empty());
		FluidGroup& fluidGroup = *block.m_fluids.at(&water).second;
		REQUIRE(fluidGroup.getBlocks().size() == 1);
		REQUIRE(fluidGroup.getBlocks().contains(&block));
		REQUIRE(blocks.fluid_getTotalVolume(block) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 1);
		REQUIRE(fluidGroup.m_excessVolume == 1);
	}
	*/
	SUBCASE("Excess volume spawns and negitive excess despawns.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		BlockIndex block = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 3);
		blocks.solid_setNot(block);
		blocks.solid_setNot(block2);
		blocks.fluid_add(block, Config::maxBlockVolume * 2u, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].block == block2);
		// Step 1.
		fluidGroup->readStep(area);
		REQUIRE(!fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].block == block3);
		REQUIRE(blocks.fluid_contains(block2, water));
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == Config::maxBlockVolume);
		REQUIRE(fluidGroup == blocks.fluid_getGroup(block2, water));
		// Step 2.
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(blocks.fluid_getGroup(block2, water));
		REQUIRE(!blocks.fluid_contains(blocks.getIndex_i(5, 5, 3), water));
		blocks.fluid_remove(block, Config::maxBlockVolume, water);
		REQUIRE(!fluidGroup->m_stable);
		// Step 3.
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(blocks.fluid_getGroup(block, water));
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == Config::maxBlockVolume);
		REQUIRE(!blocks.fluid_getGroup(block2, water));
	}
	SUBCASE("Remove volume can destroy FluidGroups.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block = blocks.getIndex_i(5, 5, 1);
		blocks.solid_setNot(block);
		blocks.fluid_add(block, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		blocks.fluid_remove(block, CollisionVolume::create(100), water);
		REQUIRE(fluidGroup->m_destroy == false);
		// Step 1.
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_futureEmpty.size() == 1);
		REQUIRE(fluidGroup->m_destroy == true);
	}
	SUBCASE("Flow into adjacent hole")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		BlockIndex destination = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex origin = blocks.getIndex_i(5, 6, 2);
		BlockIndex block4 = blocks.getIndex_i(5, 5, 3);
		BlockIndex block5 = blocks.getIndex_i(5, 6, 3);
		blocks.solid_setNot(destination);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(origin);
		blocks.fluid_add(origin, Config::maxBlockVolume, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 2);
		// Step 1.
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_destroy == false);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 2);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(block2));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(block5));
		REQUIRE(fluidGroup->m_futureRemoveFromFillQueue.empty());
		REQUIRE(fluidGroup->m_futureAddToFillQueue.size() == 3);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		REQUIRE(!blocks.fluid_getGroup(destination, water));
		REQUIRE(blocks.fluid_getGroup(block2, water));
		REQUIRE(blocks.fluid_getGroup(origin, water));
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin, water) == Config::maxBlockVolume / 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == Config::maxBlockVolume / 2);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 5);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(destination));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(block2));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(origin));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(block4));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(block5));
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		REQUIRE(fluidGroup->m_drainQueue.m_set.contains(block2));
		REQUIRE(fluidGroup->m_drainQueue.m_set.contains(origin));
		// Step 2.
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_futureRemoveFromFillQueue.size() == 4);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(blocks.fluid_getGroup(destination, water));
		REQUIRE(!blocks.fluid_getGroup(block2, water));
		REQUIRE(!blocks.fluid_getGroup(origin, water));
		REQUIRE(blocks.fluid_volumeOfTypeContains(destination, water) == Config::maxBlockVolume);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		// If the group is stable at this point depends on the viscosity of water, do one more step to make sure.
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		// Step 3.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_stable);
	}
	SUBCASE("Flow across area and then fill hole")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block = blocks.getIndex_i(5, 5, 2);
		BlockIndex block2a = blocks.getIndex_i(5, 6, 2);
		BlockIndex block2b = blocks.getIndex_i(6, 5, 2);
		BlockIndex block2c = blocks.getIndex_i(5, 4, 2);
		BlockIndex block2d = blocks.getIndex_i(4, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(6, 6, 2);
		BlockIndex block4 = blocks.getIndex_i(5, 7, 2);
		BlockIndex block5 = blocks.getIndex_i(5, 7, 1);
		BlockIndex block6 = blocks.getIndex_i(5, 8, 2);
		blocks.fluid_add(block, Config::maxBlockVolume, water);
		blocks.solid_setNot(block5);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 5);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 20);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2a, water) == 20);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2b, water) == 20);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2c, water) == 20);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2d, water) == 20);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 13);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2a, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2b, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2c, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2d, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 9);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2a, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 0);
		// If the group is stable at this point depends on the viscosity of water, do one more step to make sure.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_stable);
	}
	SUBCASE("FluidGroups are able to split into parts")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		BlockIndex destination1 = blocks.getIndex_i(5, 4, 1);
		BlockIndex destination2 = blocks.getIndex_i(5, 6, 1);
		BlockIndex origin1 = blocks.getIndex_i(5, 5, 2);
		BlockIndex origin2 = blocks.getIndex_i(5, 5, 3);
		blocks.solid_setNot(destination1);
		blocks.solid_setNot(destination2);
		blocks.solid_setNot(blocks.getBlockAbove(destination1));
		blocks.solid_setNot(blocks.getBlockAbove(destination2));
		blocks.solid_setNot(origin1);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		blocks.fluid_add(origin2, CollisionVolume::create(100), water);
		REQUIRE(blocks.fluid_getGroup(origin1, water) == blocks.fluid_getGroup(origin2, water));
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 66);
		REQUIRE(blocks.fluid_volumeOfTypeContains(blocks.getBlockAbove(destination1), water) == 66);
		REQUIRE(blocks.fluid_volumeOfTypeContains(blocks.getBlockAbove(destination2), water) == 66);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(blocks.getBlockAbove(destination1), water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(blocks.getBlockAbove(destination2), water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(destination1, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(destination2, water) == 100);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		REQUIRE(blocks.fluid_getGroup(destination1, water) != blocks.fluid_getGroup(destination2, water));
	}
	SUBCASE("Fluid Groups merge")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex origin1 = blocks.getIndex_i(5, 4, 1);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex origin2 = blocks.getIndex_i(5, 6, 1);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(origin2);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		blocks.fluid_add(origin2, CollisionVolume::create(100), water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, water);
		FluidGroup* fg2 = blocks.fluid_getGroup(origin2, water);
		REQUIRE(fg1 != fg2);
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
		fg2->mergeStep(area);
		REQUIRE(fg2->m_merged);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(fg1->m_fillQueue.m_set.size() == 6);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 50);
		REQUIRE(fg1 == blocks.fluid_getGroup(origin1, water));
		REQUIRE(fg1 == blocks.fluid_getGroup(block1, water));
		REQUIRE(fg1 == blocks.fluid_getGroup(origin2, water));
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
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 66);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 66);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 66);
		REQUIRE(fg1->m_excessVolume == 2);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Fluid Groups merge four blocks")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 4, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 6, 1);
		BlockIndex block4 = blocks.getIndex_i(5, 7, 1);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		blocks.solid_setNot(block4);
		blocks.fluid_add(block1, CollisionVolume::create(100), water);
		blocks.fluid_add(block4, CollisionVolume::create(100), water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg1 = blocks.fluid_getGroup(block1, water);
		FluidGroup* fg2 = blocks.fluid_getGroup(block4, water);
		REQUIRE(fg1 != fg2);
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
		REQUIRE(fg2->m_merged);
		fg2->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 50);
		// Step 2.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->mergeStep(area);
		fg1->splitStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 50);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Denser fluids sink")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 3);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.fluid_add(block1, CollisionVolume::create(100), water);
		blocks.fluid_add(block2, CollisionVolume::create(100), mercury);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fgWater = blocks.fluid_getGroup(block1, water);
		FluidGroup* fgMercury = blocks.fluid_getGroup(block2, mercury);
		REQUIRE(fgWater != nullptr);
		REQUIRE(fgMercury != nullptr);
		REQUIRE(fgWater->m_fluidType == water);
		REQUIRE(fgMercury->m_fluidType == mercury);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 1);
		// Step 1.
		fgWater->readStep(area);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 1);
		fgMercury->readStep(area);
		fgWater->writeStep(area);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == 1);
		fgMercury->writeStep(area);
		fgWater->afterWriteStep(area);
		fgMercury->afterWriteStep(area);
		fgWater->splitStep(area);
		fgMercury->splitStep(area);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 2);
		REQUIRE(fgWater->m_fillQueue.m_set.contains(block1));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(block2));
		fgWater->mergeStep(area);
		fgMercury->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, mercury) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, mercury) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 0);
		REQUIRE(fgWater->m_excessVolume == 50);
		REQUIRE(fgMercury->m_excessVolume == 0);
		REQUIRE(!fgWater->m_stable);
		REQUIRE(!fgMercury->m_stable);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 2);
		REQUIRE(fgWater->m_fillQueue.m_set.contains(block1));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(block2));
		REQUIRE(fgMercury->m_fillQueue.m_set.size() == 3);
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
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, mercury) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, mercury) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 0);
		REQUIRE(fgWater->m_excessVolume == 50);
		REQUIRE(fgMercury->m_excessVolume == 0);
		REQUIRE(!fgWater->m_stable);
		REQUIRE(!fgMercury->m_stable);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 3);
		REQUIRE(fgWater->m_fillQueue.m_set.contains(block1));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(block2));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(block3));
		REQUIRE(fgWater->m_drainQueue.m_set.size() == 1);
		REQUIRE(fgWater->m_drainQueue.m_set.contains(block2));
		// Step 3.
		fgWater->readStep(area);
		REQUIRE(fgWater->m_futureGroups.size() == 0);
		fgMercury->readStep(area);
		fgWater->writeStep(area);
		fgMercury->writeStep(area);
		fgWater->afterWriteStep(area);
		fgMercury->afterWriteStep(area);
		fgWater->splitStep(area);
		fgMercury->splitStep(area);
		fgWater->mergeStep(area);
		fgMercury->mergeStep(area);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == 1);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, mercury) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, mercury) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 0);
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_excessVolume == 0);
		REQUIRE(fgMercury->m_excessVolume == 0);
		REQUIRE(fgMercury->m_stable);
	}
	SUBCASE("Merge 3 groups at two block distance")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 2, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 3, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 4, 1);
		BlockIndex block4 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block5 = blocks.getIndex_i(5, 6, 1);
		BlockIndex block6 = blocks.getIndex_i(5, 7, 1);
		BlockIndex block7 = blocks.getIndex_i(5, 8, 1);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		blocks.solid_setNot(block4);
		blocks.solid_setNot(block5);
		blocks.solid_setNot(block6);
		blocks.solid_setNot(block7);
		blocks.fluid_add(block1, CollisionVolume::create(100), water);
		blocks.fluid_add(block4, CollisionVolume::create(100), water);
		blocks.fluid_add(block7, CollisionVolume::create(100), water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 3);
		FluidGroup* fg1 = blocks.fluid_getGroup(block1, water);
		FluidGroup* fg2 = blocks.fluid_getGroup(block4, water);
		FluidGroup* fg3 = blocks.fluid_getGroup(block7, water);
		REQUIRE(fg1 != nullptr);
		REQUIRE(fg2 != nullptr);
		REQUIRE(fg3 != nullptr);
		REQUIRE(fg1->m_fluidType == water);
		REQUIRE(fg2->m_fluidType == water);
		REQUIRE(fg3->m_fluidType == water);
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
		REQUIRE(fg2->m_drainQueue.m_set.size() == 7);
		REQUIRE(fg1->m_merged);
		REQUIRE(fg3->m_merged);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 33);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 33);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 33);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block7, water) == 50);
		// Step 2.
		fg2->readStep(area);
		fg2->writeStep(area);
		fg2->splitStep(area);
		fg2->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 42);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 42);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 42);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 42);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 42);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 42);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block7, water) == 42);
		fg2->readStep(area);
		REQUIRE(fg2->m_stable);
	}
	SUBCASE("Split test 2")
	{
		// Create a 3 x 1 trench with pits at each end.
		// One pit is a single block deep ( block4 ), the other is two blocks deep and includes another block on the bottom level ( blocks 1-3).
		// Expect a single fluid group with a total volume of 60 to split into two groups with volume 30.
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 4, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 2);
		BlockIndex origin1 = blocks.getIndex_i(5, 5, 3);
		BlockIndex origin2 = blocks.getIndex_i(5, 6, 3);
		BlockIndex origin3 = blocks.getIndex_i(5, 7, 3);
		BlockIndex block4 = blocks.getIndex_i(5, 7, 2);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(origin2);
		blocks.solid_setNot(origin3);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		blocks.solid_setNot(block4);
		blocks.fluid_add(origin1, CollisionVolume::create(20), water);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, water);
		blocks.fluid_add(origin2, CollisionVolume::create(20), water);
		blocks.fluid_add(origin3, CollisionVolume::create(20), water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		// Step 1.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(fg1->totalVolume(area) == 30);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 1);
		fg1 = blocks.fluid_getGroup(block3, water);
		REQUIRE(fg1->m_drainQueue.m_set.contains(block3));
		FluidGroup* fg2 = blocks.fluid_getGroup(block4, water);
		REQUIRE(fg1 != fg2);
		REQUIRE(fg2->m_drainQueue.m_set.size() == 1);
		REQUIRE(fg2->totalVolume(area) == 30);
		REQUIRE(fg1->m_fillQueue.m_set.size() == 3);
		REQUIRE(fg2->m_fillQueue.m_set.size() == 2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 30);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 30);
		// Step 2.
		fg1->readStep(area);
		fg2->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg2->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 30);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 30);
		REQUIRE(fg2->m_stable);
		// Step 3.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 15);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 15);
		// Step 4.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(fg1->m_stable);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 15);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 15);
	}
	SUBCASE("Merge with group as it splits")
	{
		// A 3x1 trench with pits on each end is created.
		// One of the pits has depth one and is empty.
		// The other has depth two and contains 100 water(fg1) at the bottom.
		// The 3x1 area contains 60 water(fg2).
		// Expect fg2 to split into two groups of 30, one of which merges with fg1.
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		BlockIndex origin1 = blocks.getIndex_i(5, 4, 1);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex origin2 = blocks.getIndex_i(5, 5, 3);
		BlockIndex origin3 = blocks.getIndex_i(5, 6, 3);
		BlockIndex origin4 = blocks.getIndex_i(5, 7, 3);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 2);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(origin2);
		blocks.solid_setNot(origin3);
		blocks.solid_setNot(origin4);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, water);
		blocks.fluid_add(origin2, CollisionVolume::create(20), water);
		FluidGroup* fg2 = blocks.fluid_getGroup(origin2, water);
		blocks.fluid_add(origin3, CollisionVolume::create(20), water);
		blocks.fluid_add(origin4, CollisionVolume::create(20), water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 1);
		REQUIRE(fg2->m_drainQueue.m_set.size() == 3);
		REQUIRE(fg1 != fg2);
		// Step 1.
		fg1->readStep(area);
		fg2->readStep(area);
		fg1->writeStep(area);
		REQUIRE(fg1->m_excessVolume == 0);
		fg2->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin4, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 30);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 30);
		// Step 2.
		fg1 = blocks.fluid_getGroup(origin1, water);
		fg2 = blocks.fluid_getGroup(block3, water);
		fg1->readStep(area);
		fg2->readStep(area);
		REQUIRE(fg1->m_excessVolume == 0);
		fg1->writeStep(area);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 2);
		fg2->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(fg1->m_excessVolume == 0);
		fg2->mergeStep(area);
		REQUIRE(!fg2->m_merged);
		REQUIRE(fg2->m_stable);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 65);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 65);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 30);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Merge with two groups while spliting")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		BlockIndex origin1 = blocks.getIndex_i(5, 4, 1);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex origin2 = blocks.getIndex_i(5, 5, 3);
		BlockIndex origin3 = blocks.getIndex_i(5, 6, 3);
		BlockIndex origin4 = blocks.getIndex_i(5, 7, 3);
		BlockIndex block3 = blocks.getIndex_i(5, 7, 2);
		BlockIndex block4 = blocks.getIndex_i(5, 7, 1);
		BlockIndex origin5 = blocks.getIndex_i(5, 8, 1);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(origin2);
		blocks.solid_setNot(origin3);
		blocks.solid_setNot(origin4);
		blocks.solid_setNot(origin5);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.solid_setNot(block3);
		blocks.solid_setNot(block4);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, water);
		blocks.fluid_add(origin2, CollisionVolume::create(20), water);
		FluidGroup* fg2 = blocks.fluid_getGroup(origin2, water);
		blocks.fluid_add(origin3, CollisionVolume::create(20), water);
		blocks.fluid_add(origin4, CollisionVolume::create(20), water);
		blocks.fluid_add(origin5, CollisionVolume::create(100), water);
		FluidGroup* fg3 = blocks.fluid_getGroup(origin5, water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 3);
		REQUIRE(fg1 != fg2);
		REQUIRE(fg1 != fg3);
		REQUIRE(fg2 != fg3);
		// Step 1.
		fg1->readStep(area);
		fg2->readStep(area);
		fg3->readStep(area);
		REQUIRE(fg3->m_futureGroups.empty());
		fg1->writeStep(area);
		REQUIRE(fg1->m_excessVolume == 0);
		fg2->writeStep(area);
		fg3->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		fg3->afterWriteStep(area);
		fg1->splitStep(area);
		fg2->splitStep(area);
		FluidGroup* fg4 = blocks.fluid_getGroup(block2, water);
		REQUIRE(!fg3->m_merged);
		fg3->splitStep(area);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 2);
		REQUIRE(fg2->m_drainQueue.m_set.size() == 1);
		fg1->mergeStep(area);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		fg2->mergeStep(area);
		fg4->mergeStep(area);
		fg3->mergeStep(area);
		REQUIRE(fg4->m_merged);
		REQUIRE(fg2->m_merged);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(fg4->m_drainQueue.m_set.size() == 1);
		REQUIRE(fg1 != fg4);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin4, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 30);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 30);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin5, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 50);
		// Step 2.
		fg1->readStep(area);
		fg3->readStep(area);
		REQUIRE(fg1->m_excessVolume == 0);
		fg1->writeStep(area);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 2);
		fg3->writeStep(area);
		fg1->afterWriteStep(area);
		fg3->afterWriteStep(area);
		fg1->splitStep(area);
		fg3->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(fg1->m_excessVolume == 0);
		fg3->mergeStep(area);
		REQUIRE(fg3->m_stable);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 65);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 65);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin5, water) == 65);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 65);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Bubbles")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		BlockIndex origin1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex origin2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex origin3 = blocks.getIndex_i(5, 5, 3);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 4);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(origin2);
		blocks.solid_setNot(origin3);
		blocks.solid_setNot(block1);
		blocks.fluid_add(origin1, CollisionVolume::create(100), CO2);
		blocks.fluid_add(origin2, CollisionVolume::create(100), water);
		blocks.fluid_add(origin3, CollisionVolume::create(100), water);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, CO2);
		FluidGroup* fg2 = blocks.fluid_getGroup(origin2, water);
		// Step 1.
		fg1->readStep(area);
		fg2->readStep(area);
		fg1->writeStep(area);
		fg2->writeStep(area);
		fg1->afterWriteStep(area);
		fg2->afterWriteStep(area);
		REQUIRE(fg1->m_excessVolume == 100);
		REQUIRE(fg1->m_disolved);
		fg2->splitStep(area);
		fg2->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, CO2) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, CO2) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, CO2) == 100);
		REQUIRE(fg1 == blocks.fluid_getGroup(origin3, CO2));
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
		REQUIRE(fg1->m_stable);
		fg2->removeFluid(area, CollisionVolume::create(100));
		// Step 3.
		fg2->readStep(area);
		fg2->writeStep(area);
		REQUIRE(!fg1->m_stable);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		fg2->splitStep(area);
		fg2->mergeStep(area);
		// Step 4.
		fg1->readStep(area);
		REQUIRE(fg2->m_stable);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, CO2) == 100);
		// Step 5.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		REQUIRE(fg1->m_stable);
		REQUIRE(fg2->m_stable);
	}
	SUBCASE("Three liquids")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		BlockIndex origin1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex origin2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex origin3 = blocks.getIndex_i(5, 5, 3);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 4);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(origin2);
		blocks.solid_setNot(origin3);
		blocks.solid_setNot(block1);
		blocks.fluid_add(origin1, CollisionVolume::create(100), CO2);
		blocks.fluid_add(origin2, CollisionVolume::create(100), water);
		blocks.fluid_add(origin3, CollisionVolume::create(100), mercury);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, CO2);
		FluidGroup* fg2 = blocks.fluid_getGroup(origin2, water);
		FluidGroup* fg3 = blocks.fluid_getGroup(origin3, mercury);
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
		REQUIRE(fg1->m_excessVolume == 100);
		REQUIRE(fg1->m_disolved);
		fg2->splitStep(area);
		REQUIRE(fg1->m_excessVolume == 50);
		fg3->splitStep(area);
		fg2->mergeStep(area);
		fg3->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, CO2) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, CO2) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, mercury) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, mercury) == 50);
		REQUIRE(fg1 == blocks.fluid_getGroup(origin2, CO2));
		REQUIRE(fg1->m_excessVolume == 50);
		// Step 2.
		fg1->readStep(area);
		fg2->readStep(area);
		fg3->readStep(area);
		REQUIRE(!fg3->m_stable);
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
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, mercury) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, mercury) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, CO2) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, CO2) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, mercury) == 0);
		REQUIRE(blocks.fluid_getTotalVolume(block1) == 0);
		REQUIRE(fg2->m_excessVolume == 50);
		REQUIRE(fg1->m_excessVolume == 0);
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
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, mercury) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, mercury) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, CO2) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, CO2) == 50);
		REQUIRE(blocks.fluid_getTotalVolume(block1) == 0);
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(fg2->m_excessVolume == 50);
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
		REQUIRE(fg2->m_stable);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, mercury) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 100);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin3, CO2) == 50);
		REQUIRE(blocks.fluid_getTotalVolume(block1) == 0);
		REQUIRE(fg1->m_excessVolume == 50);
		REQUIRE(fg2->m_excessVolume == 0);
		REQUIRE(fg2->m_stable);
		REQUIRE(fg3->m_stable);
		// Step 5.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(fg1->m_stable);
		REQUIRE(fg3->m_stable);
	}
	SUBCASE("Set not solid")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex origin1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block1 = blocks.getIndex_i(5, 6, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 7, 1);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(block2);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, water);
		REQUIRE(fg1 != nullptr);
		// Step 1.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 100);
		REQUIRE(fg1->m_stable);
		// Step 2.
		blocks.solid_setNot(block1);
		REQUIRE(!fg1->m_stable);
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		// Step .
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 33);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 33);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 33);
		REQUIRE(!fg1->m_stable);
		REQUIRE(fg1->m_excessVolume == 1);
	}
	SUBCASE("Set solid")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex origin1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block1 = blocks.getIndex_i(5, 6, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 7, 1);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, water);
		// Step 1.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(fg1->m_excessVolume == 0);
		blocks.solid_set(block1, marble, false);
	}
	SUBCASE("Set solid and split")
	{
		// Create a 3x1 trench and put 100 water in the middle block.
		// Allow it to spread out to 33 in each block with 1 excessVolume.
		// Set the middle block solid and verify that the fluid group splits into two groups, each containing 50.
		//
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex origin1 = blocks.getIndex_i(5, 6, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 7, 1);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(origin1);
		blocks.solid_setNot(block2);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		FluidGroup* fg1 = blocks.fluid_getGroup(origin1, water);
		// Step 1.
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 33);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 33);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 33);
		REQUIRE(fg1->m_excessVolume == 1);
		// Step 2.
		blocks.solid_set(origin1, marble, false);
		REQUIRE(blocks.solid_is(origin1));
		REQUIRE(fg1->m_potentiallySplitFromSyncronusStep.size() == 2);
		REQUIRE(fg1->m_potentiallySplitFromSyncronusStep.contains(block1));
		REQUIRE(fg1->m_potentiallySplitFromSyncronusStep.contains(block2));
		REQUIRE(fg1->m_excessVolume == 34);
		fg1->readStep(area);
		fg1->writeStep(area);
		fg1->afterWriteStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 50);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 50);
		fg1->splitStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg2 = &area.m_hasFluidGroups.getAll().back();
		fg1->mergeStep(area);
		fg2->mergeStep(area);
		//Step 3.
		assert(area.m_hasFluidGroups.getAll().size() == 2);
		fg1 = blocks.fluid_getGroup(block1, water);
		fg2 = blocks.fluid_getGroup(block2, water);
		if(!fg1->m_stable)
			fg1->readStep(area);
		if(!fg2->m_stable)
			fg2->readStep(area);
		REQUIRE(fg2->m_stable);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Cave in falls in fluid and pistons it up")
	{
		if constexpr(!Config::fluidPiston)
			return
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		blocks.solid_setNot(block1);
		blocks.fluid_add(block1, CollisionVolume::create(100), water);
		blocks.solid_set(block2, marble, false);
		area.m_caveInCheck.add(block2);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		area.stepCaveInRead();
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		area.stepCaveInWrite();
		REQUIRE(area.m_hasFluidGroups.getUnstable().size() == 1);
		REQUIRE(blocks.fluid_getTotalVolume(block1) == 0);
		REQUIRE(blocks.solid_get(block1) == marble);
		REQUIRE(!blocks.solid_is(block2));
		REQUIRE(fluidGroup->m_excessVolume == 100);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 0);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(block2));
		REQUIRE(blocks.fluid_canEnterEver(block2));
		fluidGroup->readStep(area);
		area.stepCaveInRead();
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		area.stepCaveInWrite();
		REQUIRE(blocks.fluid_getTotalVolume(block2) == 100);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		REQUIRE(fluidGroup->m_stable == false);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
	}
	SUBCASE("Test diagonal seep")
	{
		if constexpr(Config::fluidsSeepDiagonalModifier == 0)
			return;
		simulation.m_step = Step::create(1);
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(6, 6, 1);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.fluid_add(block1, CollisionVolume::create(10), water);
		FluidGroup* fg1 = *area.m_hasFluidGroups.getUnstable().begin();
		fg1->readStep(area);
		fg1->writeStep(area);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 0);
		fg1->afterWriteStep(area);
		fg1->splitStep(area);
		fg1->mergeStep(area);
		++simulation.m_step;
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 1);
		FluidGroup* fg2 = blocks.fluid_getGroup(block2, water);
		REQUIRE(fg1 != fg2);
		REQUIRE(fg1->m_excessVolume == -1);
		REQUIRE(!fg1->m_stable);
		for(int i = 0; i < 5; ++i)
		{
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
			++simulation.m_step;
		}
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 5);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 5);
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(!fg1->m_stable);
	}
	SUBCASE("Test mist")
	{
		simulation.m_step = Step::create(1);
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		BlockIndex block1 = blocks.getIndex_i(5, 5, 1);
		BlockIndex block2 = blocks.getIndex_i(5, 5, 2);
		BlockIndex block3 = blocks.getIndex_i(5, 5, 3);
		BlockIndex block4 = blocks.getIndex_i(5, 5, 4);
		BlockIndex block5 = blocks.getIndex_i(5, 6, 3);
		blocks.solid_setNot(block1);
		blocks.solid_setNot(block2);
		blocks.fluid_add(block3, CollisionVolume::create(100), water);
		blocks.fluid_add(block4, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		// Step 1.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(blocks.fluid_getMist(block5) == water);
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
		REQUIRE(blocks.fluid_getMist(block5).empty());
	}
}
TEST_CASE("area larger")
{
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static FluidTypeId water = FluidType::byName(L"water");
	Simulation simulation;
	Area& area = simulation.m_hasAreas->createArea(20,20,20);
	area.m_hasRain.disable();
	Blocks& blocks = area.getBlocks();
	SUBCASE("Flow across flat area double stack")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex origin1 = blocks.getIndex_i(10, 10, 1);
		BlockIndex origin2 = blocks.getIndex_i(10, 10, 2);
		BlockIndex block1 = blocks.getIndex_i(10, 11, 1);
		BlockIndex block2 = blocks.getIndex_i(11, 11, 1);
		BlockIndex block3 = blocks.getIndex_i(10, 12, 1);
		BlockIndex block4 = blocks.getIndex_i(10, 13, 1);
		BlockIndex block5 = blocks.getIndex_i(10, 14, 1);
		BlockIndex block6 = blocks.getIndex_i(15, 10, 1);
		BlockIndex block7 = blocks.getIndex_i(16, 10, 1);
		blocks.fluid_add(origin1, CollisionVolume::create(100), water);
		blocks.fluid_add(origin2, CollisionVolume::create(100), water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 5);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 40);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin2, water) == 0);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 40);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 13);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 15);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 15);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 15);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 15);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 5);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 25);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 25);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 41);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 4);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 4);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 4);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 4);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 4);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 36);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 61);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block7, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 17);
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 85);
		REQUIRE(blocks.fluid_volumeOfTypeContains(origin1, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block1, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block7, water) == 2);
		REQUIRE(fluidGroup->m_excessVolume == 30);
		REQUIRE(!fluidGroup->m_stable);
	}
	SUBCASE("Flow across flat area")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		BlockIndex block = blocks.getIndex_i(10, 10, 1);
		BlockIndex block2 = blocks.getIndex_i(10, 12, 1);
		BlockIndex block3 = blocks.getIndex_i(11, 11, 1);
		BlockIndex block4 = blocks.getIndex_i(10, 13, 1);
		BlockIndex block5 = blocks.getIndex_i(10, 14, 1);
		BlockIndex block6 = blocks.getIndex_i(10, 15, 1);
		BlockIndex block7 = blocks.getIndex_i(16, 10, 1);
		BlockIndex block8 = blocks.getIndex_i(17, 10, 1);
		blocks.fluid_add(block, Config::maxBlockVolume, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		//Step 1.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 5);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 20);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 0);
		for(BlockIndex adjacent : blocks.getDirectlyAdjacent(block))
			if(blocks.getZ(adjacent) == 1)
				REQUIRE(blocks.fluid_volumeOfTypeContains(adjacent, water) == 20);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		++simulation.m_step;
		//Step 2.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 13);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 7);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 9);
		++simulation.m_step;
		//Step 3.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 25);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 3);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 25);
		++simulation.m_step;
		//Step 4.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 41);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 2);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 18);
		++simulation.m_step;
		//Step 5.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 61);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block8, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 39);
		++simulation.m_step;
		//Step 6.
		fluidGroup->readStep(area);
		fluidGroup->writeStep(area);
		fluidGroup->afterWriteStep(area);
		fluidGroup->splitStep(area);
		fluidGroup->mergeStep(area);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 85);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block2, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block3, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block4, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block5, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block6, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block7, water) == 1);
		REQUIRE(blocks.fluid_volumeOfTypeContains(block8, water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 15);
		++simulation.m_step;
		//Step 7.
		fluidGroup->readStep(area);
		REQUIRE(fluidGroup->m_stable);
	}
}
TEST_CASE("fluids multi scale")
{
	static MaterialTypeId marble = MaterialType::byName(L"marble");
	static FluidTypeId water = FluidType::byName(L"water");
	static FluidTypeId CO2 = FluidType::byName(L"CO2");
	static FluidTypeId mercury = FluidType::byName(L"mercury");
	static FluidTypeId lava = FluidType::byName(L"lava");
	Simulation simulation;
	auto trenchTest2Fluids = [&](uint32_t scaleL, uint32_t scaleW, Step steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t halfMaxX = maxX / 2;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Blocks& blocks = area.getBlocks();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(halfMaxX - 1, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		BlockIndex CO2_1 = blocks.getIndex_i(halfMaxX, 1, 1);
		BlockIndex CO2_2 = blocks.getIndex_i(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = blocks.fluid_getGroup(CO2_1, CO2);
		REQUIRE(!fgWater->m_merged);
		REQUIRE(!fgCO2->m_merged);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			++simulation.m_step;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = 1 + (maxZ - 1) / 2;
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgWater->totalVolume(area) == totalVolume);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume(area) == totalVolume);
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, 1), water));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(maxX - 2, 1, 1), water));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, maxZ - 1), CO2));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(maxX - 2, 1, maxZ - 1), CO2));
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
	auto trenchTest3Fluids = [&](uint32_t scaleL, uint32_t scaleW, Step steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t thirdMaxX = maxX / 3;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Blocks& blocks = area.getBlocks();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(thirdMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		BlockIndex CO2_1 = blocks.getIndex_i(thirdMaxX + 1, 1, 1);
		BlockIndex CO2_2 = blocks.getIndex_i((thirdMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Lava
		BlockIndex lava1 = blocks.getIndex_i((thirdMaxX * 2) + 1, 1, 1);
		BlockIndex lava2 = blocks.getIndex_i(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 3);
		FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = blocks.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = blocks.fluid_getGroup(lava1, lava);
		simulation.m_step = Step::create(1);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			++simulation.m_step;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = std::max(1u, maxZ / 3);
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgWater->totalVolume(area) == totalVolume);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume(area) == totalVolume);
		REQUIRE(fgLava->m_stable);
		REQUIRE(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgLava->totalVolume(area) == totalVolume);
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, 1), lava));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(maxX - 2, 1, 1), lava));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, maxZ - 1), CO2));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(maxX - 2, 1, maxZ - 1), CO2));
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
	auto trenchTest4Fluids = [&](uint32_t scaleL, uint32_t scaleW, Step steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t quarterMaxX = maxX / 4;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Blocks& blocks = area.getBlocks();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(quarterMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		BlockIndex CO2_1 = blocks.getIndex_i(quarterMaxX + 1, 1, 1);
		BlockIndex CO2_2 = blocks.getIndex_i((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Lava
		BlockIndex lava1 = blocks.getIndex_i((quarterMaxX * 2) + 1, 1, 1);
		BlockIndex lava2 = blocks.getIndex_i(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
		// Mercury
		BlockIndex mercury1 = blocks.getIndex_i((quarterMaxX * 3) + 1, 1, 1);
		BlockIndex mercury2 = blocks.getIndex_i(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = blocks.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = blocks.fluid_getGroup(lava1, lava);
		FluidGroup* fgMercury = blocks.fluid_getGroup(mercury1, mercury);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
			if(fgMercury != nullptr)
				REQUIRE(fgMercury->totalVolume(area) == totalVolume);
			++simulation.m_step;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = std::max(1u, maxZ / 4);
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		fgWater = areaBuilderUtil::getFluidGroup(area, water);
		REQUIRE(fgWater != nullptr);
		fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
		REQUIRE(fgCO2 != nullptr);
		fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
		REQUIRE(fgMercury != nullptr);
		fgLava = areaBuilderUtil::getFluidGroup(area, lava);
		REQUIRE(fgLava != nullptr);
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgWater->totalVolume(area) == totalVolume);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume(area) == totalVolume);
		REQUIRE(fgLava->m_stable);
		REQUIRE(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgLava->totalVolume(area) == totalVolume);
		REQUIRE(fgMercury->m_stable);
		REQUIRE(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgMercury->totalVolume(area) == totalVolume);
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, 1), mercury));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(maxX - 2, 1, 1), mercury));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, maxZ - 1), CO2));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(maxX - 2, 1, maxZ - 1), CO2));
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		REQUIRE(area.m_hasFluidGroups.getUnstable().empty());
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
		trenchTest4Fluids(16, 4, Step::create(25));
	}
	auto trenchTest2FluidsMerge = [&](uint32_t scaleL, uint32_t scaleW, Step steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t quarterMaxX = maxX / 4;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Blocks& blocks = area.getBlocks();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(quarterMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		BlockIndex CO2_1 = blocks.getIndex_i(quarterMaxX + 1, 1, 1);
		BlockIndex CO2_2 = blocks.getIndex_i((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Water
		BlockIndex water3 = blocks.getIndex_i((quarterMaxX * 2) + 1, 1, 1);
		BlockIndex water4 = blocks.getIndex_i(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water3, water4, water);
		// CO2
		BlockIndex CO2_3 = blocks.getIndex_i((quarterMaxX * 3) + 1, 1, 1);
		BlockIndex CO2_4 = blocks.getIndex_i(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_3, CO2_4, CO2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
		CollisionVolume totalVolume = fgWater->totalVolume(area) * 2u;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			++simulation.m_step;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = std::max(1u, maxZ / 2);
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(area.m_hasFluidGroups.getUnstable().empty());
		fgWater = blocks.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = blocks.fluid_getGroup(water2, CO2);
		REQUIRE(fgWater->totalVolume(area) == totalVolume);
		REQUIRE(fgCO2->totalVolume(area) == totalVolume);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
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
	auto trenchTest3FluidsMerge = [&](uint32_t scaleL, uint32_t scaleW, Step steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t quarterMaxX = maxX / 4;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Blocks& blocks = area.getBlocks();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(quarterMaxX, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2
		BlockIndex CO2_1 = blocks.getIndex_i(quarterMaxX + 1, 1, 1);
		BlockIndex CO2_2 = blocks.getIndex_i((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Mercury
		BlockIndex mercury1 = blocks.getIndex_i((quarterMaxX * 2) + 1, 1, 1);
		BlockIndex mercury2 = blocks.getIndex_i(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
		// CO2
		BlockIndex CO2_3 = blocks.getIndex_i((quarterMaxX * 3) + 1, 1, 1);
		BlockIndex CO2_4 = blocks.getIndex_i(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_3, CO2_4, CO2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
		CollisionVolume totalVolumeWater = fgWater->totalVolume(area);
		CollisionVolume totalVolumeMercury = totalVolumeWater;
		CollisionVolume totalVolumeCO2 = totalVolumeWater * 2u;
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			++simulation.m_step;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = std::max(1u, maxZ / 4);
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(area.m_hasFluidGroups.getUnstable().empty());
		fgWater = areaBuilderUtil::getFluidGroup(area, water);
		FluidGroup* fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
		FluidGroup* fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
		REQUIRE(fgWater != nullptr);
		REQUIRE(fgCO2 != nullptr);
		REQUIRE(fgMercury != nullptr);
		REQUIRE(fgWater->totalVolume(area) == totalVolumeWater);
		REQUIRE(fgCO2->totalVolume(area) == totalVolumeCO2);
		REQUIRE(fgMercury->totalVolume(area) == totalVolumeMercury);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE((fgCO2->m_drainQueue.m_set.size() == expectedBlocks || fgCO2->m_drainQueue.m_set.size() == expectedBlocks * 2));
		REQUIRE(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgMercury->m_stable);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 3);
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
	auto fourFluidsTest = [&](uint32_t scale, Step steps)
	{
		uint32_t maxX = (scale * 2) + 2;
		uint32_t maxY = (scale * 2) + 2;
		uint32_t maxZ = (scale * 1) + 1;
		uint32_t halfMaxX = maxX / 2;
		uint32_t halfMaxY = maxY / 2;
		Area& area = simulation.m_hasAreas->createArea(maxX, maxY, maxZ);
		area.m_hasRain.disable();
		Blocks& blocks = area.getBlocks();
		simulation.m_step = Step::create(0);
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		std::vector<FluidGroup*> newlySplit;
		// Water is at 0,0
		BlockIndex water1 = blocks.getIndex_i(1, 1, 1);
		BlockIndex water2 = blocks.getIndex_i(halfMaxX - 1, halfMaxY - 1, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, water1, water2, water);
		// CO2 is at 0,1
		BlockIndex CO2_1 = blocks.getIndex_i(1, halfMaxY, 1);
		BlockIndex CO2_2 = blocks.getIndex_i(halfMaxX - 1, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, CO2_1, CO2_2, CO2);
		// Lava is at 1,0
		BlockIndex lava1 = blocks.getIndex_i(halfMaxX, 1, 1);
		BlockIndex lava2 = blocks.getIndex_i(maxX - 2, halfMaxY - 1, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, lava1, lava2, lava);
		// Mercury is at 1,1
		BlockIndex mercury1 = blocks.getIndex_i(halfMaxX, halfMaxY, 1);
		BlockIndex mercury2 = blocks.getIndex_i(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(area, mercury1, mercury2, mercury);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = blocks.fluid_getGroup(water1, water);
		FluidGroup* fgCO2 = blocks.fluid_getGroup(CO2_1, CO2);
		FluidGroup* fgLava = blocks.fluid_getGroup(lava1, lava);
		FluidGroup* fgMercury = blocks.fluid_getGroup(mercury1, mercury);
		REQUIRE(!fgWater->m_merged);
		REQUIRE(!fgCO2->m_merged);
		REQUIRE(!fgLava->m_merged);
		REQUIRE(!fgMercury->m_merged);
		CollisionVolume totalVolume = fgWater->totalVolume(area);
		simulation.m_step = Step::create(1);
		while(simulation.m_step < steps)
		{
			area.m_hasFluidGroups.doStep(false);
			++simulation.m_step;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = ((maxZ - 2) / 4) + 1;
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
		fgWater = areaBuilderUtil::getFluidGroup(area, water);
		fgLava = areaBuilderUtil::getFluidGroup(area, lava);
		fgCO2 = areaBuilderUtil::getFluidGroup(area, CO2);
		REQUIRE(fgWater != nullptr);
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgWater->totalVolume(area) == totalVolume);
		REQUIRE(fgCO2 != nullptr);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume(area) == totalVolume);
		REQUIRE(fgLava != nullptr);
		REQUIRE(fgLava->m_stable);
		REQUIRE(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgLava->totalVolume(area) == totalVolume);
		REQUIRE(fgMercury != nullptr);
		REQUIRE(fgMercury->m_stable);
		REQUIRE(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgMercury->totalVolume(area) == totalVolume);
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, 1), mercury));
		REQUIRE(blocks.fluid_getGroup(blocks.getIndex_i(1, 1, maxZ - 1), CO2));
	};
	SUBCASE("four fluids scale 2")
	{
		fourFluidsTest(2, Step::create(10));
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
