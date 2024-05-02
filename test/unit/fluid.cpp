#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/fluidGroup.h"
#include "../../engine/materialType.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
TEST_CASE("fluids smaller")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const FluidType& water = FluidType::byName("water");
	static const FluidType& CO2 = FluidType::byName("CO2");
	static const FluidType& mercury = FluidType::byName("mercury");
	Simulation simulation;
	Area& area = simulation.createArea(10,10,10);
	SUBCASE("Create Fluid 100")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block = area.getBlock(5, 5, 1);
		block.setNotSolid();
		block.m_hasFluids.addFluid(100, water);
		REQUIRE(area.getBlock(5, 5, 1).m_hasFluids.contains(water));
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->mergeStep();
		fluidGroup->splitStep();
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(!block.getBlockSouth()->m_hasFluids.contains(water));
		REQUIRE(!block.getBlockEast()->m_hasFluids.contains(water));
		REQUIRE(!block.getBlockNorth()->m_hasFluids.contains(water));
		REQUIRE(!block.getBlockWest()->m_hasFluids.contains(water));
		REQUIRE(!block.getBlockBelow()->m_hasFluids.contains(water));
		REQUIRE(!block.getBlockAbove()->m_hasFluids.contains(water));
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
	}
	/*
	SUBCASE("Create Fluid 2")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 4);
		block.setNotSolid();
		block.m_hasFluids.addFluid(1, water);
		block2.m_hasFluids.addFluid(1, water);
		REQUIRE(block.m_hasFluids.contains(&water));
		simulation.doStep();
		simulation.doStep();
		REQUIRE(block.m_hasFluids.contains(&water));
		REQUIRE(area.m_hasFluidGroups.getUnstable().empty());
		FluidGroup& fluidGroup = *block.m_fluids.at(&water).second;
		REQUIRE(fluidGroup.getBlocks().size() == 1);
		REQUIRE(fluidGroup.getBlocks().contains(&block));
		REQUIRE(block.m_hasFluids.getTotalVolume() == 1);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(fluidGroup.m_excessVolume == 1);
	}
	*/
	SUBCASE("Excess volume spawns and negitive excess despawns.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Block& block = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		Block& block3 = area.getBlock(5, 5, 3);
		block.setNotSolid();
		block2.setNotSolid();
		block.m_hasFluids.addFluid(Config::maxBlockVolume * 2, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].block == &block2);
		// Step 1.
		fluidGroup->readStep();
		REQUIRE(!fluidGroup->m_stable);
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_queue[0].block == &block3);
		REQUIRE(block2.m_hasFluids.contains(water));
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == Config::maxBlockVolume);
		REQUIRE(fluidGroup == block2.m_hasFluids.getFluidGroup(water));
		// Step 2.
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(block2.m_hasFluids.contains(water));
		REQUIRE(!area.getBlock(5, 5, 3).m_hasFluids.contains(water));
		block.m_hasFluids.removeFluid(Config::maxBlockVolume, water);
		REQUIRE(!fluidGroup->m_stable);
		// Step 3.
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(block.m_hasFluids.contains(water));
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == Config::maxBlockVolume);
		REQUIRE(!block2.m_hasFluids.contains(water));
	}
	SUBCASE("Remove volume can destroy FluidGroups.")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block = area.getBlock(5, 5, 1);
		block.setNotSolid();
		block.m_hasFluids.addFluid(100, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_stable);
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		block.m_hasFluids.removeFluid(100, water);
		REQUIRE(fluidGroup->m_destroy == false);
		// Step 1.
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_futureEmpty.size() == 1);
		REQUIRE(fluidGroup->m_destroy == true);
	}
	SUBCASE("Flow into adjacent hole")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Block& destination = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		Block& origin = area.getBlock(5, 6, 2);
		Block& block4 = area.getBlock(5, 5, 3);
		Block& block5 = area.getBlock(5, 6, 3);
		destination.setNotSolid();
		block2.setNotSolid();
		origin.setNotSolid();
		origin.m_hasFluids.addFluid(Config::maxBlockVolume, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 2);
		// Step 1.
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_destroy == false);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 2);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&block2));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&block5));
		REQUIRE(fluidGroup->m_futureRemoveFromFillQueue.empty());
		REQUIRE(fluidGroup->m_futureAddToFillQueue.size() == 3);
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		REQUIRE(!destination.m_hasFluids.contains(water));
		REQUIRE(block2.m_hasFluids.contains(water));
		REQUIRE(origin.m_hasFluids.contains(water));
		REQUIRE(origin.m_hasFluids.volumeOfFluidTypeContains(water) == Config::maxBlockVolume / 2);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == Config::maxBlockVolume / 2);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 5);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&destination));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&block2));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&origin));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&block4));
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&block5));
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		REQUIRE(fluidGroup->m_drainQueue.m_set.contains(&block2));
		REQUIRE(fluidGroup->m_drainQueue.m_set.contains(&origin));
		// Step 2.
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_futureRemoveFromFillQueue.size() == 4);
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(destination.m_hasFluids.contains(water));
		REQUIRE(!block2.m_hasFluids.contains(water));
		REQUIRE(!origin.m_hasFluids.contains(water));
		REQUIRE(destination.m_hasFluids.volumeOfFluidTypeContains(water) == Config::maxBlockVolume);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		// If the group is stable at this point depends on the viscosity of water, do one more step to make sure.
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		// Step 3.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_stable);
	}
	SUBCASE("Flow across area and then fill hole")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block = area.getBlock(5, 5, 2);
		Block& block2a = area.getBlock(5, 6, 2);
		Block& block2b = area.getBlock(6, 5, 2);
		Block& block2c = area.getBlock(5, 4, 2);
		Block& block2d = area.getBlock(4, 5, 2);
		Block& block3 = area.getBlock(6, 6, 2);
		Block& block4 = area.getBlock(5, 7, 2);
		Block& block5 = area.getBlock(5, 7, 1);
		Block& block6 = area.getBlock(5, 8, 2);
		block.m_hasFluids.addFluid(Config::maxBlockVolume, water);
		block5.setNotSolid();
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 5);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 20);
		REQUIRE(block2a.m_hasFluids.volumeOfFluidTypeContains(water) == 20);
		REQUIRE(block2b.m_hasFluids.volumeOfFluidTypeContains(water) == 20);
		REQUIRE(block2c.m_hasFluids.volumeOfFluidTypeContains(water) == 20);
		REQUIRE(block2d.m_hasFluids.volumeOfFluidTypeContains(water) == 20);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 13);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block2a.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block2b.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block2c.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block2d.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 9);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 1);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block2a.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		// If the group is stable at this point depends on the viscosity of water, do one more step to make sure.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_stable);
	}
	SUBCASE("FluidGroups are able to split into parts")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Block& destination1 = area.getBlock(5, 4, 1);
		Block& destination2 = area.getBlock(5, 6, 1);
		Block& origin1 = area.getBlock(5, 5, 2);
		Block& origin2 = area.getBlock(5, 5, 3);
		destination1.setNotSolid();
		destination2.setNotSolid();
		destination1.m_adjacents[5]->setNotSolid();
		destination2.m_adjacents[5]->setNotSolid();
		origin1.setNotSolid();
		origin1.m_hasFluids.addFluid(100, water);
		origin2.m_hasFluids.addFluid(100, water);
		REQUIRE(origin1.m_hasFluids.getFluidGroup(water) == origin2.m_hasFluids.getFluidGroup(water));
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 66);
		REQUIRE(destination1.m_adjacents[5]->m_hasFluids.volumeOfFluidTypeContains(water) == 66);
		REQUIRE(destination2.m_adjacents[5]->m_hasFluids.volumeOfFluidTypeContains(water) == 66);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(destination1.m_adjacents[5]->m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(destination2.m_adjacents[5]->m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(destination1.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(destination2.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		REQUIRE(destination1.m_hasFluids.getFluidGroup(water) != destination2.m_hasFluids.getFluidGroup(water));
	}
	SUBCASE("Fluid Groups merge")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& origin1 = area.getBlock(5, 4, 1);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& origin2 = area.getBlock(5, 6, 1);
		origin1.setNotSolid();
		block1.setNotSolid();
		origin2.setNotSolid();
		origin1.m_hasFluids.addFluid(100, water);
		origin2.m_hasFluids.addFluid(100, water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fg2 = origin2.m_hasFluids.getFluidGroup(water);
		REQUIRE(fg1 != fg2);
		// Step 1.
		fg1->readStep();
		fg2->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		REQUIRE(fg2->m_merged);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(fg1->m_fillQueue.m_set.size() == 6);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(fg1 == origin1.m_hasFluids.getFluidGroup(water));
		REQUIRE(fg1 == block1.m_hasFluids.getFluidGroup(water));
		REQUIRE(fg1 == origin2.m_hasFluids.getFluidGroup(water));
		// Step 2.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		// Step 3.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 66);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 66);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 66);
		REQUIRE(fg1->m_excessVolume == 2);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Fluid Groups merge four blocks")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block1 = area.getBlock(5, 4, 1);
		Block& block2 = area.getBlock(5, 5, 1);
		Block& block3 = area.getBlock(5, 6, 1);
		Block& block4 = area.getBlock(5, 7, 1);
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		block4.setNotSolid();
		block1.m_hasFluids.addFluid(100, water);
		block4.m_hasFluids.addFluid(100, water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg1 = block1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fg2 = block4.m_hasFluids.getFluidGroup(water);
		REQUIRE(fg1 != fg2);
		// Step 1.
		fg1->readStep();
		fg2->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg1->mergeStep();
		REQUIRE(fg2->m_merged);
		fg2->mergeStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		// Step 2.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->mergeStep();
		fg1->splitStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Denser fluids sink")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		Block& block3 = area.getBlock(5, 5, 3);
		block1.setNotSolid();
		block2.setNotSolid();
		block1.m_hasFluids.addFluid(100, water);
		block2.m_hasFluids.addFluid(100, mercury);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fgWater = block1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fgMercury = block2.m_hasFluids.getFluidGroup(mercury);
		REQUIRE(fgWater != nullptr);
		REQUIRE(fgMercury != nullptr);
		REQUIRE(fgWater->m_fluidType == water);
		REQUIRE(fgMercury->m_fluidType == mercury);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 1);
		// Step 1.
		fgWater->readStep();
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 1);
		fgMercury->readStep();
		fgWater->writeStep();
		REQUIRE(fgWater->m_drainQueue.m_set.size() == 1);
		fgMercury->writeStep();
		fgWater->afterWriteStep();
		fgMercury->afterWriteStep();
		fgWater->splitStep();
		fgMercury->splitStep();
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 2);
		REQUIRE(fgWater->m_fillQueue.m_set.contains(&block1));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(&block2));
		fgWater->mergeStep();
		fgMercury->mergeStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(mercury) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(mercury) == 50);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fgWater->m_excessVolume == 50);
		REQUIRE(fgMercury->m_excessVolume == 0);
		REQUIRE(!fgWater->m_stable);
		REQUIRE(!fgMercury->m_stable);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 2);
		REQUIRE(fgWater->m_fillQueue.m_set.contains(&block1));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(&block2));
		REQUIRE(fgMercury->m_fillQueue.m_set.size() == 3);
		// Step 2.
		fgWater->readStep();
		fgMercury->readStep();
		fgWater->writeStep();
		fgMercury->writeStep();
		fgWater->afterWriteStep();
		fgMercury->afterWriteStep();
		fgWater->splitStep();
		fgMercury->splitStep();
		fgWater->mergeStep();
		fgMercury->mergeStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(mercury) == 100);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(mercury) == 0);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fgWater->m_excessVolume == 50);
		REQUIRE(fgMercury->m_excessVolume == 0);
		REQUIRE(!fgWater->m_stable);
		REQUIRE(!fgMercury->m_stable);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 3);
		REQUIRE(fgWater->m_fillQueue.m_set.contains(&block1));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(&block2));
		REQUIRE(fgWater->m_fillQueue.m_set.contains(&block3));
		REQUIRE(fgWater->m_drainQueue.m_set.size() == 1);
		REQUIRE(fgWater->m_drainQueue.m_set.contains(&block2));
		// Step 3.
		fgWater->readStep();
		REQUIRE(fgWater->m_futureGroups.size() == 0);
		fgMercury->readStep();
		fgWater->writeStep();
		fgMercury->writeStep();
		fgWater->afterWriteStep();
		fgMercury->afterWriteStep();
		fgWater->splitStep();
		fgMercury->splitStep();
		fgWater->mergeStep();
		fgMercury->mergeStep();
		REQUIRE(fgWater->m_drainQueue.m_set.size() == 1);
		REQUIRE(fgWater->m_fillQueue.m_set.size() == 2);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(mercury) == 100);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(mercury) == 0);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_excessVolume == 0);
		REQUIRE(fgMercury->m_excessVolume == 0);
		REQUIRE(fgMercury->m_stable);
	}
	SUBCASE("Merge 3 groups at two block distance")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block1 = area.getBlock(5, 2, 1);
		Block& block2 = area.getBlock(5, 3, 1);
		Block& block3 = area.getBlock(5, 4, 1);
		Block& block4 = area.getBlock(5, 5, 1);
		Block& block5 = area.getBlock(5, 6, 1);
		Block& block6 = area.getBlock(5, 7, 1);
		Block& block7 = area.getBlock(5, 8, 1);
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		block4.setNotSolid();
		block5.setNotSolid();
		block6.setNotSolid();
		block7.setNotSolid();
		block1.m_hasFluids.addFluid(100, water);
		block4.m_hasFluids.addFluid(100, water);
		block7.m_hasFluids.addFluid(100, water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 3);
		FluidGroup* fg1 = block1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fg2 = block4.m_hasFluids.getFluidGroup(water);
		FluidGroup* fg3 = block7.m_hasFluids.getFluidGroup(water);
		REQUIRE(fg1 != nullptr);
		REQUIRE(fg2 != nullptr);
		REQUIRE(fg3 != nullptr);
		REQUIRE(fg1->m_fluidType == water);
		REQUIRE(fg2->m_fluidType == water);
		REQUIRE(fg3->m_fluidType == water);
		// Step 1.
		fg1->readStep();
		fg2->readStep();
		fg3->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg3->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg3->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg3->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		fg3->mergeStep();
		REQUIRE(fg2->m_drainQueue.m_set.size() == 7);
		REQUIRE(fg1->m_merged);
		REQUIRE(fg3->m_merged);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block7.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		// Step 2.
		fg2->readStep();
		fg2->writeStep();
		fg2->splitStep();
		fg2->mergeStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 42);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 42);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 42);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 42);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 42);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 42);
		REQUIRE(block7.m_hasFluids.volumeOfFluidTypeContains(water) == 42);
		fg2->readStep();
		REQUIRE(fg2->m_stable);
	}
	SUBCASE("Split test 2")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Block& block1 = area.getBlock(5, 4, 1);
		Block& block2 = area.getBlock(5, 5, 1);
		Block& block3 = area.getBlock(5, 5, 2);
		Block& origin1 = area.getBlock(5, 5, 3);
		Block& origin2 = area.getBlock(5, 6, 3);
		Block& origin3 = area.getBlock(5, 7, 3);
		Block& block4 = area.getBlock(5, 7, 2);
		origin1.setNotSolid();
		origin2.setNotSolid();
		origin3.setNotSolid();
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		block4.setNotSolid();
		origin1.m_hasFluids.addFluid(20, water);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(water);
		origin2.m_hasFluids.addFluid(20, water);
		origin3.m_hasFluids.addFluid(20, water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		// Step 1.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 1);
		REQUIRE(fg1->m_drainQueue.m_set.contains(&block4));
		fg1 = block3.m_hasFluids.getFluidGroup(water);
		FluidGroup* fg2 = block4.m_hasFluids.getFluidGroup(water);
		REQUIRE(fg2->m_drainQueue.m_set.size() == 1);
		REQUIRE(fg1 != fg2);
		REQUIRE(fg1->m_fillQueue.m_set.size() == 3);
		REQUIRE(fg2->m_fillQueue.m_set.size() == 2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		// Step 2.
		fg1->readStep();
		fg2->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg2->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		REQUIRE(fg2->m_stable);
		// Step 3.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
		// Step 4.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(fg1->m_stable);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
	}
	SUBCASE("Merge with group as it splits")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Block& origin1 = area.getBlock(5, 4, 1);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		Block& origin2 = area.getBlock(5, 5, 3);
		Block& origin3 = area.getBlock(5, 6, 3);
		Block& origin4 = area.getBlock(5, 7, 3);
		Block& block3 = area.getBlock(5, 7, 2);
		origin1.setNotSolid();
		origin2.setNotSolid();
		origin3.setNotSolid();
		origin4.setNotSolid();
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		origin1.m_hasFluids.addFluid(100, water);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(water);
		origin2.m_hasFluids.addFluid(20, water);
		FluidGroup* fg2 = origin2.m_hasFluids.getFluidGroup(water);
		origin3.m_hasFluids.addFluid(20, water);
		origin4.m_hasFluids.addFluid(20, water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 1);
		REQUIRE(fg2->m_drainQueue.m_set.size() == 3);
		REQUIRE(fg1 != fg2);
		// Step 1.
		fg1->readStep();
		fg2->readStep();
		fg1->writeStep();
		REQUIRE(fg1->m_excessVolume == 0);
		fg2->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin4.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		// Step 2.
		fg1->readStep();
		fg2->readStep();
		REQUIRE(fg1->m_excessVolume == 0);
		fg1->writeStep();
		REQUIRE(fg1->m_drainQueue.m_set.size() == 2);
		fg2->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg1->mergeStep();
		REQUIRE(fg1->m_excessVolume == 0);
		fg2->mergeStep();
		REQUIRE(!fg2->m_merged);
		REQUIRE(fg2->m_stable);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 65);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 65);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Merge with two groups while spliting")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Block& origin1 = area.getBlock(5, 4, 1);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		Block& origin2 = area.getBlock(5, 5, 3);
		Block& origin3 = area.getBlock(5, 6, 3);
		Block& origin4 = area.getBlock(5, 7, 3);
		Block& block3 = area.getBlock(5, 7, 2);
		Block& block4 = area.getBlock(5, 7, 1);
		Block& origin5 = area.getBlock(5, 8, 1);
		origin1.setNotSolid();
		origin2.setNotSolid();
		origin3.setNotSolid();
		origin4.setNotSolid();
		origin5.setNotSolid();
		block1.setNotSolid();
		block2.setNotSolid();
		block3.setNotSolid();
		block4.setNotSolid();
		origin1.m_hasFluids.addFluid(100, water);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(water);
		origin2.m_hasFluids.addFluid(20, water);
		FluidGroup* fg2 = origin2.m_hasFluids.getFluidGroup(water);
		origin3.m_hasFluids.addFluid(20, water);
		origin4.m_hasFluids.addFluid(20, water);
		origin5.m_hasFluids.addFluid(100, water);
		FluidGroup* fg3 = origin5.m_hasFluids.getFluidGroup(water);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 3);
		REQUIRE(fg1 != fg2);
		REQUIRE(fg1 != fg3);
		REQUIRE(fg2 != fg3);
		// Step 1.
		fg1->readStep();
		fg2->readStep();
		fg3->readStep();
		REQUIRE(fg3->m_futureGroups.empty());
		fg1->writeStep();
		REQUIRE(fg1->m_excessVolume == 0);
		fg2->writeStep();
		fg3->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg3->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		FluidGroup* fg4 = block2.m_hasFluids.getFluidGroup(water);
		REQUIRE(!fg3->m_merged);
		fg3->splitStep();
		REQUIRE(fg1->m_drainQueue.m_set.size() == 2);
		REQUIRE(fg2->m_drainQueue.m_set.size() == 1);
		fg1->mergeStep();
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		fg2->mergeStep();
		fg4->mergeStep();
		fg3->mergeStep();
		REQUIRE(fg4->m_merged);
		REQUIRE(fg2->m_merged);
		REQUIRE(fg1->m_drainQueue.m_set.size() == 3);
		REQUIRE(fg4->m_drainQueue.m_set.size() == 1);
		REQUIRE(fg1 != fg4);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin4.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 30);
		REQUIRE(origin5.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		// Step 2.
		fg1->readStep();
		fg3->readStep();
		REQUIRE(fg1->m_excessVolume == 0);
		fg1->writeStep();
		REQUIRE(fg1->m_drainQueue.m_set.size() == 2);
		fg3->writeStep();
		fg1->afterWriteStep();
		fg3->afterWriteStep();
		fg1->splitStep();
		fg3->splitStep();
		fg1->mergeStep();
		REQUIRE(fg1->m_excessVolume == 0);
		fg3->mergeStep();
		REQUIRE(fg3->m_stable);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 65);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 65);
		REQUIRE(origin5.m_hasFluids.volumeOfFluidTypeContains(water) == 65);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 65);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Bubbles")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Block& origin1 = area.getBlock(5, 5, 1);
		Block& origin2 = area.getBlock(5, 5, 2);
		Block& origin3 = area.getBlock(5, 5, 3);
		Block& block1 = area.getBlock(5, 5, 4);
		origin1.setNotSolid();
		origin2.setNotSolid();
		origin3.setNotSolid();
		block1.setNotSolid();
		origin1.m_hasFluids.addFluid(100, CO2);
		origin2.m_hasFluids.addFluid(100, water);
		origin3.m_hasFluids.addFluid(100, water);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(CO2);
		FluidGroup* fg2 = origin2.m_hasFluids.getFluidGroup(water);
		// Step 1.
		fg1->readStep();
		fg2->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		REQUIRE(fg1->m_excessVolume == 100);
		REQUIRE(fg1->m_disolved);
		fg2->splitStep();
		fg2->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(CO2) == 0);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(CO2) == 0);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(CO2) == 100);
		REQUIRE(fg1 == origin3.m_hasFluids.getFluidGroup(CO2));
		// Step 2.
		fg1->readStep();
		fg2->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		REQUIRE(fg1->m_stable);
		fg2->removeFluid(100);
		// Step 3.
		fg2->readStep();
		fg2->writeStep();
		REQUIRE(!fg1->m_stable);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		fg2->splitStep();
		fg2->mergeStep();
		// Step 4.
		fg1->readStep();
		REQUIRE(fg2->m_stable);
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(CO2) == 100);
		// Step 5.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		REQUIRE(fg1->m_stable);
		REQUIRE(fg2->m_stable);
	}
	SUBCASE("Three liquids")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 4, marble);
		Block& origin1 = area.getBlock(5, 5, 1);
		Block& origin2 = area.getBlock(5, 5, 2);
		Block& origin3 = area.getBlock(5, 5, 3);
		Block& block1 = area.getBlock(5, 5, 4);
		origin1.setNotSolid();
		origin2.setNotSolid();
		origin3.setNotSolid();
		block1.setNotSolid();
		origin1.m_hasFluids.addFluid(100, CO2);
		origin2.m_hasFluids.addFluid(100, water);
		origin3.m_hasFluids.addFluid(100, mercury);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(CO2);
		FluidGroup* fg2 = origin2.m_hasFluids.getFluidGroup(water);
		FluidGroup* fg3 = origin3.m_hasFluids.getFluidGroup(mercury);
		// Step 1.
		fg1->readStep();
		fg2->readStep();
		fg3->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg3->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg3->afterWriteStep();
		REQUIRE(fg1->m_excessVolume == 100);
		REQUIRE(fg1->m_disolved);
		fg2->splitStep();
		REQUIRE(fg1->m_excessVolume == 50);
		fg3->splitStep();
		fg2->mergeStep();
		fg3->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(CO2) == 0);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(CO2) == 50);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(mercury) == 50);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(mercury) == 50);
		REQUIRE(fg1 == origin2.m_hasFluids.getFluidGroup(CO2));
		REQUIRE(fg1->m_excessVolume == 50);
		// Step 2.
		fg1->readStep();
		fg2->readStep();
		fg3->readStep();
		REQUIRE(!fg3->m_stable);
		fg1->writeStep();
		fg2->writeStep();
		fg3->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg3->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg3->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		fg3->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(mercury) == 50);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(mercury) == 50);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(CO2) == 50);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(CO2) == 50);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(mercury) == 0);
		REQUIRE(block1.m_hasFluids.getTotalVolume() == 0);
		REQUIRE(fg2->m_excessVolume == 50);
		REQUIRE(fg1->m_excessVolume == 0);
		// Step 3.
		fg1->readStep();
		fg2->readStep();
		fg3->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg3->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg3->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg3->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		fg3->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(mercury) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(mercury) == 0);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(CO2) == 50);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(CO2) == 50);
		REQUIRE(block1.m_hasFluids.getTotalVolume() == 0);
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(fg2->m_excessVolume == 50);
		// Step 4.
		fg1->readStep();
		fg2->readStep();
		fg3->readStep();
		fg1->writeStep();
		fg2->writeStep();
		fg3->writeStep();
		fg1->afterWriteStep();
		fg2->afterWriteStep();
		fg3->afterWriteStep();
		fg1->splitStep();
		fg2->splitStep();
		fg3->splitStep();
		fg1->mergeStep();
		fg2->mergeStep();
		fg3->mergeStep();
		REQUIRE(fg2->m_stable);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(mercury) == 100);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(origin3.m_hasFluids.volumeOfFluidTypeContains(CO2) == 50);
		REQUIRE(block1.m_hasFluids.getTotalVolume() == 0);
		REQUIRE(fg1->m_excessVolume == 50);
		REQUIRE(fg2->m_excessVolume == 0);
		REQUIRE(fg2->m_stable);
		REQUIRE(fg3->m_stable);
		// Step 5.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(fg1->m_stable);
		REQUIRE(fg3->m_stable);
	}
	SUBCASE("Set not solid")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& origin1 = area.getBlock(5, 5, 1);
		Block& block1 = area.getBlock(5, 6, 1);
		Block& block2 = area.getBlock(5, 7, 1);
		origin1.setNotSolid();
		block2.setNotSolid();
		origin1.m_hasFluids.addFluid(100, water);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(water);
		REQUIRE(fg1 != nullptr);
		// Step 1.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 100);
		REQUIRE(fg1->m_stable);
		// Step 2.
		block1.setNotSolid();
		REQUIRE(!fg1->m_stable);
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		// Step .
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(!fg1->m_stable);
		REQUIRE(fg1->m_excessVolume == 1);
	}
	SUBCASE("Set solid")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& origin1 = area.getBlock(5, 5, 1);
		Block& block1 = area.getBlock(5, 6, 1);
		Block& block2 = area.getBlock(5, 7, 1);
		origin1.setNotSolid();
		block1.setNotSolid();
		block2.setNotSolid();
		origin1.m_hasFluids.addFluid(100, water);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(water);
		// Step 1.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(fg1->m_excessVolume == 0);
		block1.setSolid(marble);
	}
	SUBCASE("Set solid and split")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& origin1 = area.getBlock(5, 6, 1);
		Block& block2 = area.getBlock(5, 7, 1);
		block1.setNotSolid();
		origin1.setNotSolid();
		block2.setNotSolid();
		origin1.m_hasFluids.addFluid(100, water);
		FluidGroup* fg1 = origin1.m_hasFluids.getFluidGroup(water);
		// Step 1.
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 33);
		REQUIRE(fg1->m_excessVolume == 1);
		// Step 2.
		origin1.setSolid(marble);
		REQUIRE(origin1.isSolid());
		REQUIRE(fg1->m_potentiallySplitFromSyncronusStep.size() == 2);
		REQUIRE(fg1->m_potentiallySplitFromSyncronusStep.contains(&block1));
		REQUIRE(fg1->m_potentiallySplitFromSyncronusStep.contains(&block2));
		REQUIRE(fg1->m_excessVolume == 34);
		fg1->readStep();
		fg1->writeStep();
		fg1->afterWriteStep();
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 50);
		fg1->splitStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fg2 = &area.m_hasFluidGroups.getAll().back();
		fg1->mergeStep();
		fg2->mergeStep();
		//Step 3.
		fg1->readStep();
		fg2->readStep();
		REQUIRE(fg2->m_stable);
		REQUIRE(fg1->m_stable);
	}
	SUBCASE("Cave in falls in fluid and pistons it up")
	{
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		block1.setNotSolid();
		block1.m_hasFluids.addFluid(100, water);
		block2.setSolid(marble);
		area.m_caveInCheck.insert(&block2);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		area.stepCaveInRead();
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		area.stepCaveInWrite();
		REQUIRE(area.m_hasFluidGroups.getUnstable().size() == 1);
		REQUIRE(block1.m_hasFluids.getTotalVolume() == 0);
		REQUIRE(block1.getSolidMaterial() == marble);
		REQUIRE(!block2.isSolid());
		REQUIRE(fluidGroup->m_excessVolume == 100);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 0);
		REQUIRE(fluidGroup->m_fillQueue.m_set.size() == 1);
		REQUIRE(fluidGroup->m_fillQueue.m_set.contains(&block2));
		REQUIRE(block2.m_hasFluids.fluidCanEnterEver());
		fluidGroup->readStep();
		area.stepCaveInRead();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		area.stepCaveInWrite();
		REQUIRE(block2.m_hasFluids.getTotalVolume() == 100);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		REQUIRE(fluidGroup->m_stable == false);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
	}
	SUBCASE("Test diagonal seep")
	{
		if constexpr(Config::fluidsSeepDiagonalModifier == 0)
			return;
		simulation.m_step = 1;
		areaBuilderUtil::setSolidLayers(area, 0, 1, marble);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(6, 6, 1);
		block1.setNotSolid();
		block2.setNotSolid();
		block1.m_hasFluids.addFluid(10, water);
		FluidGroup* fg1 = *area.m_hasFluidGroups.getUnstable().begin();
		fg1->readStep();
		fg1->writeStep();
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		fg1->afterWriteStep();
		fg1->splitStep();
		fg1->mergeStep();
		simulation.m_step++;
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		FluidGroup* fg2 = block2.m_hasFluids.getFluidGroup(water);
		REQUIRE(fg1 != fg2);
		REQUIRE(fg1->m_excessVolume == -1);
		REQUIRE(!fg1->m_stable);
		for(int i = 0; i < 5; ++i)
		{
			fg1->readStep();
			fg2->readStep();
			fg1->writeStep();
			fg2->writeStep();
			fg1->afterWriteStep();
			fg2->afterWriteStep();
			fg1->splitStep();
			fg2->splitStep();
			fg1->mergeStep();
			fg2->mergeStep();
			simulation.m_step++;
		}
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 5);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 5);
		REQUIRE(fg1->m_excessVolume == 0);
		REQUIRE(!fg1->m_stable);
	}
	SUBCASE("Test mist")
	{
		simulation.m_step = 1;
		areaBuilderUtil::setSolidLayers(area, 0, 2, marble);
		Block& block1 = area.getBlock(5, 5, 1);
		Block& block2 = area.getBlock(5, 5, 2);
		Block& block3 = area.getBlock(5, 5, 3);
		Block& block4 = area.getBlock(5, 5, 4);
		Block& block5 = area.getBlock(5, 6, 3);
		block1.setNotSolid();
		block2.setNotSolid();
		block3.m_hasFluids.addFluid(100, water);
		block4.m_hasFluids.addFluid(100, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		// Step 1.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(block5.m_hasFluids.getMist() == &water);
		// Several steps.
		while(simulation.m_step < 11)
		{
			if(!fluidGroup->m_stable)
			{
				fluidGroup->readStep();
				fluidGroup->writeStep();
				fluidGroup->afterWriteStep();
				fluidGroup->splitStep();
				fluidGroup->mergeStep();
			}
			simulation.m_step++;
		}
		simulation.m_eventSchedule.execute(11);
		REQUIRE(block5.m_hasFluids.getMist() == nullptr);
	}
}
TEST_CASE("area larger")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation;
	Area& area = simulation.createArea(20,20,20);
	SUBCASE("Flow across flat area double stack")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& origin1 = area.getBlock(10, 10, 1);
		Block& origin2 = area.getBlock(10, 10, 2);
		Block& block1 = area.getBlock(10, 11, 1);
		Block& block2 = area.getBlock(11, 11, 1);
		Block& block3 = area.getBlock(10, 12, 1);
		Block& block4 = area.getBlock(10, 13, 1);
		Block& block5 = area.getBlock(10, 14, 1);
		Block& block6 = area.getBlock(15, 10, 1);
		Block& block7 = area.getBlock(16, 10, 1);
		origin1.m_hasFluids.addFluid(100, water);
		origin2.m_hasFluids.addFluid(100, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 2);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 5);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 40);
		REQUIRE(origin2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 40);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 13);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 15);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 5);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 25);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 25);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 41);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 4);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 4);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 4);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 4);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 4);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 36);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 61);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block7.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 17);
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 85);
		REQUIRE(origin1.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block1.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block7.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(fluidGroup->m_excessVolume == 30);
		REQUIRE(!fluidGroup->m_stable);
	}
	SUBCASE("Flow across flat area")
	{
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		Block& block = area.getBlock(10, 10, 1);
		Block& block2 = area.getBlock(10, 12, 1);
		Block& block3 = area.getBlock(11, 11, 1);
		Block& block4 = area.getBlock(10, 13, 1);
		Block& block5 = area.getBlock(10, 14, 1);
		Block& block6 = area.getBlock(10, 15, 1);
		Block& block7 = area.getBlock(16, 10, 1);
		Block& block8 = area.getBlock(17, 10, 1);
		block.m_hasFluids.addFluid(Config::maxBlockVolume, water);
		FluidGroup* fluidGroup = *area.m_hasFluidGroups.getUnstable().begin();
		//Step 1.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 5);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 20);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		for(Block* adjacent : block.m_adjacents)
			if(adjacent->m_z == 1)
				REQUIRE(adjacent->m_hasFluids.volumeOfFluidTypeContains(water) == 20);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 0);
		simulation.m_step++;
		//Step 2.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 13);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 7);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 9);
		simulation.m_step++;
		//Step 3.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 25);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 3);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(!fluidGroup->m_stable);
		REQUIRE(fluidGroup->m_excessVolume == 25);
		simulation.m_step++;
		//Step 4.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 41);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 2);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 18);
		simulation.m_step++;
		//Step 5.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 61);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block8.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 39);
		simulation.m_step++;
		//Step 6.
		fluidGroup->readStep();
		fluidGroup->writeStep();
		fluidGroup->afterWriteStep();
		fluidGroup->splitStep();
		fluidGroup->mergeStep();
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 1);
		REQUIRE(fluidGroup->m_drainQueue.m_set.size() == 85);
		REQUIRE(block.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block2.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block3.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block4.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block5.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block6.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block7.m_hasFluids.volumeOfFluidTypeContains(water) == 1);
		REQUIRE(block8.m_hasFluids.volumeOfFluidTypeContains(water) == 0);
		REQUIRE(fluidGroup->m_excessVolume == 15);
		simulation.m_step++;
		//Step 7.
		fluidGroup->readStep();
		REQUIRE(fluidGroup->m_stable);
	}
}
TEST_CASE("fluids multi scale")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const FluidType& water = FluidType::byName("water");
	static const FluidType& CO2 = FluidType::byName("CO2");
	static const FluidType& mercury = FluidType::byName("mercury");
	static const FluidType& lava = FluidType::byName("lava");
	Simulation simulation;
	auto trenchTest2Fluids = [&](uint32_t scaleL, uint32_t scaleW, uint32_t steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t halfMaxX = maxX / 2;
		Area& area = simulation.createArea(maxX, maxY, maxZ);
		simulation.m_step = 0;
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(halfMaxX - 1, maxY - 2, maxZ - 1);		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		// CO2
		Block& CO2_1 = area.getBlock(halfMaxX, 1, 1);
		Block& CO2_2 = area.getBlock(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_1, CO2_2, CO2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
		FluidGroup* fgWater = water1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fgCO2 = CO2_1.m_hasFluids.getFluidGroup(CO2);
		REQUIRE(!fgWater->m_merged);
		REQUIRE(!fgCO2->m_merged);
		uint32_t totalVolume = fgWater->totalVolume();
		simulation.m_step = 1;
		while(simulation.m_step < steps)
		{
			for(FluidGroup* fluidGroup : area.m_hasFluidGroups.getUnstable())
				fluidGroup->readStep();
			area.writeStep();
			simulation.m_step++;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = 1 + (maxZ - 1) / 2;
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgWater->totalVolume() == totalVolume);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume() == totalVolume);
		REQUIRE(area.getBlock(1, 1, 1).m_hasFluids.contains(water));
		REQUIRE(area.getBlock(maxX - 2, 1, 1).m_hasFluids.contains(water));
		REQUIRE(area.getBlock(1, 1, maxZ - 1).m_hasFluids.contains(CO2));
		REQUIRE(area.getBlock(maxX - 2, 1, maxZ - 1).m_hasFluids.contains(CO2));
	};
	SUBCASE("trench test 2 fluids scale 2-1")
	{
		trenchTest2Fluids(2, 1, 8);
	}
	SUBCASE("trench test 2 fluids scale 4-1")
	{
		trenchTest2Fluids(4, 1, 12);
	}
	SUBCASE("trench test 2 fluids scale 40-1")
	{
		trenchTest2Fluids(40, 1, 30);
	}
	SUBCASE("trench test 2 fluids scale 20-5")
	{
		trenchTest2Fluids(20, 5, 20);
	}
	auto trenchTest3Fluids = [&](uint32_t scaleL, uint32_t scaleW, uint32_t steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t thirdMaxX = maxX / 3;
		Area& area = simulation.createArea(maxX, maxY, maxZ);
		simulation.m_step = 0;
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(thirdMaxX, maxY - 2, maxZ - 1);		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		// CO2
		Block& CO2_1 = area.getBlock(thirdMaxX + 1, 1, 1);
		Block& CO2_2 = area.getBlock((thirdMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_1, CO2_2, CO2);
		// Lava
		Block& lava1 = area.getBlock((thirdMaxX * 2) + 1, 1, 1);
		Block& lava2 = area.getBlock(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(lava1, lava2, lava);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 3);
		FluidGroup* fgWater = water1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fgCO2 = CO2_1.m_hasFluids.getFluidGroup(CO2);
		FluidGroup* fgLava = lava1.m_hasFluids.getFluidGroup(lava);
		simulation.m_step = 1;
		uint32_t totalVolume = fgWater->totalVolume();
		while(simulation.m_step < steps)
		{
			for(FluidGroup* fluidGroup : area.m_hasFluidGroups.getUnstable())
				fluidGroup->readStep();
			area.writeStep();
			simulation.m_step++;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = std::max(1u, maxZ / 3);
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(fgWater->m_stable);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgWater->totalVolume() == totalVolume);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume() == totalVolume);
		REQUIRE(fgLava->m_stable);
		REQUIRE(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgLava->totalVolume() == totalVolume);
		REQUIRE(area.getBlock(1, 1, 1).m_hasFluids.contains(lava));
		REQUIRE(area.getBlock(maxX - 2, 1, 1).m_hasFluids.contains(lava));
		REQUIRE(area.getBlock(1, 1, maxZ - 1).m_hasFluids.contains(CO2));
		REQUIRE(area.getBlock(maxX - 2, 1, maxZ - 1).m_hasFluids.contains(CO2));
	};
	SUBCASE("trench test 3 fluids scale 3-1")
	{
		trenchTest3Fluids(3, 1, 10);
	}
	SUBCASE("trench test 3 fluids scale 9-1")
	{
		trenchTest3Fluids(9, 1, 20);
	}
	SUBCASE("trench test 3 fluids scale 3-3")
	{
		trenchTest3Fluids(3, 3, 10);
	}
	SUBCASE("trench test 3 fluids scale 18-3")
	{
		trenchTest3Fluids(18, 3, 30);
	}
	auto trenchTest4Fluids = [&](uint32_t scaleL, uint32_t scaleW, uint32_t steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t quarterMaxX = maxX / 4;
		Area& area = simulation.createArea(maxX, maxY, maxZ);
		simulation.m_step = 0;
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(quarterMaxX, maxY - 2, maxZ - 1);		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		// CO2
		Block& CO2_1 = area.getBlock(quarterMaxX + 1, 1, 1);
		Block& CO2_2 = area.getBlock((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_1, CO2_2, CO2);
		// Lava
		Block& lava1 = area.getBlock((quarterMaxX * 2) + 1, 1, 1);
		Block& lava2 = area.getBlock(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(lava1, lava2, lava);
		// Mercury
		Block& mercury1 = area.getBlock((quarterMaxX * 3) + 1, 1, 1);
		Block& mercury2 = area.getBlock(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(mercury1, mercury2, mercury);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = water1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fgCO2 = CO2_1.m_hasFluids.getFluidGroup(CO2);
		FluidGroup* fgLava = lava1.m_hasFluids.getFluidGroup(lava);
		FluidGroup* fgMercury = mercury1.m_hasFluids.getFluidGroup(mercury);
		uint32_t totalVolume = fgWater->totalVolume();
		simulation.m_step = 1;
		while(simulation.m_step < steps)
		{
			for(FluidGroup* fluidGroup : area.m_hasFluidGroups.getUnstable())
				fluidGroup->readStep();
			area.writeStep();
			fgMercury = areaBuilderUtil::getFluidGroup(area, mercury);
			if(fgMercury != nullptr)
				REQUIRE(fgMercury->totalVolume() == totalVolume);
			simulation.m_step++;
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
		REQUIRE(fgWater->totalVolume() == totalVolume);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume() == totalVolume);
		REQUIRE(fgLava->m_stable);
		REQUIRE(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgLava->totalVolume() == totalVolume);
		REQUIRE(fgMercury->m_stable);
		REQUIRE(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgMercury->totalVolume() == totalVolume);
		REQUIRE(area.getBlock(1, 1, 1).m_hasFluids.contains(mercury));
		REQUIRE(area.getBlock(maxX - 2, 1, 1).m_hasFluids.contains(mercury));
		REQUIRE(area.getBlock(1, 1, maxZ - 1).m_hasFluids.contains(CO2));
		REQUIRE(area.getBlock(maxX - 2, 1, maxZ - 1).m_hasFluids.contains(CO2));
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		REQUIRE(area.m_hasFluidGroups.getUnstable().empty());
	};
	SUBCASE("trench test 4 fluids scale 4-1")
	{
		trenchTest4Fluids(4, 1, 10);
	}
	SUBCASE("trench test 4 fluids scale 4-2")
	{
		trenchTest4Fluids(4, 2, 10);
	}
	SUBCASE("trench test 4 fluids scale 4-4")
	{
		trenchTest4Fluids(4, 4, 15);
	}
	SUBCASE("trench test 4 fluids scale 4-8")
	{
		trenchTest4Fluids(4, 8, 17);
	}
	SUBCASE("trench test 4 fluids scale 8-8")
	{
		trenchTest4Fluids(8, 8, 20);
	}
	SUBCASE("trench test 4 fluids scale 16-4")
	{
		trenchTest4Fluids(16, 4, 25);
	}
	auto trenchTest2FluidsMerge = [&](uint32_t scaleL, uint32_t scaleW, uint32_t steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t quarterMaxX = maxX / 4;
		Area& area = simulation.createArea(maxX, maxY, maxZ);
		simulation.m_step = 0;
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(quarterMaxX, maxY - 2, maxZ - 1);		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		// CO2
		Block& CO2_1 = area.getBlock(quarterMaxX + 1, 1, 1);
		Block& CO2_2 = area.getBlock((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_1, CO2_2, CO2);
		// Water
		Block& water3 = area.getBlock((quarterMaxX * 2) + 1, 1, 1);
		Block& water4 = area.getBlock(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(water3, water4, water);
		// CO2
		Block& CO2_3 = area.getBlock((quarterMaxX * 3) + 1, 1, 1);
		Block& CO2_4 = area.getBlock(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_3, CO2_4, CO2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = water1.m_hasFluids.getFluidGroup(water);
		uint32_t totalVolume = fgWater->totalVolume() * 2;
		simulation.m_step = 1;
		while(simulation.m_step < steps)
		{
			for(FluidGroup* fluidGroup : area.m_hasFluidGroups.getUnstable())
				fluidGroup->readStep();
			area.writeStep();
			simulation.m_step++;
		}
		uint32_t totalBlocks2D = (maxX - 2) * (maxY - 2);
		uint32_t expectedHeight = std::max(1u, maxZ / 2);
		uint32_t expectedBlocks = totalBlocks2D * expectedHeight;
		REQUIRE(area.m_hasFluidGroups.getUnstable().empty());
		fgWater = water1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fgCO2 = water2.m_hasFluids.getFluidGroup(CO2);
		REQUIRE(fgWater->totalVolume() == totalVolume);
		REQUIRE(fgCO2->totalVolume() == totalVolume);
		REQUIRE(fgWater->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 2);
	};
	SUBCASE("trench test 2 fluids merge scale 4-1")
	{
		trenchTest2FluidsMerge(4, 1, 6);
	}
	SUBCASE("trench test 2 fluids merge scale 4-4")
	{
		trenchTest2FluidsMerge(4, 4, 6);
	}
	SUBCASE("trench test 2 fluids merge scale 8-4")
	{
		trenchTest2FluidsMerge(8, 4, 8);
	}
	SUBCASE("trench test 2 fluids merge scale 16-4")
	{
		trenchTest2FluidsMerge(16, 4, 12);
	}
	auto trenchTest3FluidsMerge = [&](uint32_t scaleL, uint32_t scaleW, uint32_t steps)
	{
		uint32_t maxX = scaleL + 2;
		uint32_t maxY = scaleW + 2;
		uint32_t maxZ = scaleW + 1;
		uint32_t quarterMaxX = maxX / 4;
		Area& area = simulation.createArea(maxX, maxY, maxZ);
		simulation.m_step = 0;
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		// Water
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(quarterMaxX, maxY - 2, maxZ - 1);		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		// CO2
		Block& CO2_1 = area.getBlock(quarterMaxX + 1, 1, 1);
		Block& CO2_2 = area.getBlock((quarterMaxX * 2), maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_1, CO2_2, CO2);
		// Mercury
		Block& mercury1 = area.getBlock((quarterMaxX * 2) + 1, 1, 1);
		Block& mercury2 = area.getBlock(quarterMaxX * 3, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(mercury1, mercury2, mercury);
		// CO2
		Block& CO2_3 = area.getBlock((quarterMaxX * 3) + 1, 1, 1);
		Block& CO2_4 = area.getBlock(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_3, CO2_4, CO2);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = water1.m_hasFluids.getFluidGroup(water);
		uint32_t totalVolumeWater = fgWater->totalVolume();
		uint32_t totalVolumeMercury = totalVolumeWater;
		uint32_t totalVolumeCO2 = totalVolumeWater * 2;
		simulation.m_step = 1;
		while(simulation.m_step < steps)
		{
			for(FluidGroup* fluidGroup : area.m_hasFluidGroups.getUnstable())
				fluidGroup->readStep();
			area.writeStep();
			simulation.m_step++;
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
		REQUIRE(fgWater->totalVolume() == totalVolumeWater);
		REQUIRE(fgCO2->totalVolume() == totalVolumeCO2);
		REQUIRE(fgMercury->totalVolume() == totalVolumeMercury);
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
		trenchTest3FluidsMerge(4, 1, 8);
	}
	SUBCASE("trench test 3 fluids merge scale 4-4")
	{
		trenchTest3FluidsMerge(4, 4, 10);
	}
	SUBCASE("trench test 3 fluids merge scale 8-4")
	{
		trenchTest3FluidsMerge(8, 4, 15);
	}
	SUBCASE("trench test 3 fluids merge scale 16-4")
	{
		trenchTest3FluidsMerge(16, 4, 24);
	}
	auto fourFluidsTest = [&](uint32_t scale, uint32_t steps)
	{
		uint32_t maxX = (scale * 2) + 2;
		uint32_t maxY = (scale * 2) + 2;
		uint32_t maxZ = (scale * 1) + 1;
		uint32_t halfMaxX = maxX / 2;
		uint32_t halfMaxY = maxY / 2;
		Area& area = simulation.createArea(maxX, maxY, maxZ);
		simulation.m_step = 0;
		areaBuilderUtil::setSolidLayer(area, 0, marble);
		areaBuilderUtil::setSolidWalls(area, maxZ - 1, marble);
		std::vector<FluidGroup*> newlySplit;
		// Water is at 0,0
		Block& water1 = area.getBlock(1, 1, 1);
		Block& water2 = area.getBlock(halfMaxX - 1, halfMaxY - 1, maxZ - 1);		
		areaBuilderUtil::setFullFluidCuboid(water1, water2, water);
		// CO2 is at 0,1
		Block& CO2_1 = area.getBlock(1, halfMaxY, 1);
		Block& CO2_2 = area.getBlock(halfMaxX - 1, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(CO2_1, CO2_2, CO2);
		// Lava is at 1,0
		Block& lava1 = area.getBlock(halfMaxX, 1, 1);
		Block& lava2 = area.getBlock(maxX - 2, halfMaxY - 1, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(lava1, lava2, lava);
		// Mercury is at 1,1
		Block& mercury1 = area.getBlock(halfMaxX, halfMaxY, 1);
		Block& mercury2 = area.getBlock(maxX - 2, maxY - 2, maxZ - 1);
		areaBuilderUtil::setFullFluidCuboid(mercury1, mercury2, mercury);
		REQUIRE(area.m_hasFluidGroups.getAll().size() == 4);
		FluidGroup* fgWater = water1.m_hasFluids.getFluidGroup(water);
		FluidGroup* fgCO2 = CO2_1.m_hasFluids.getFluidGroup(CO2);
		FluidGroup* fgLava = lava1.m_hasFluids.getFluidGroup(lava);
		FluidGroup* fgMercury = mercury1.m_hasFluids.getFluidGroup(mercury);
		REQUIRE(!fgWater->m_merged);
		REQUIRE(!fgCO2->m_merged);
		REQUIRE(!fgLava->m_merged);
		REQUIRE(!fgMercury->m_merged);
		uint32_t totalVolume = fgWater->totalVolume();
		simulation.m_step = 1;
		while(simulation.m_step < steps)
		{
			for(FluidGroup* fluidGroup : area.m_hasFluidGroups.getUnstable())
				fluidGroup->readStep();
			area.writeStep();
			simulation.m_step++;
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
		REQUIRE(fgWater->totalVolume() == totalVolume);
		REQUIRE(fgCO2 != nullptr);
		REQUIRE(fgCO2->m_stable);
		REQUIRE(fgCO2->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgCO2->totalVolume() == totalVolume);
		REQUIRE(fgLava != nullptr);
		REQUIRE(fgLava->m_stable);
		REQUIRE(fgLava->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgLava->totalVolume() == totalVolume);
		REQUIRE(fgMercury != nullptr);
		REQUIRE(fgMercury->m_stable);
		REQUIRE(fgMercury->m_drainQueue.m_set.size() == expectedBlocks);
		REQUIRE(fgMercury->totalVolume() == totalVolume);
		REQUIRE(area.getBlock(1, 1, 1).m_hasFluids.contains(mercury));
		REQUIRE(area.getBlock(1, 1, maxZ - 1).m_hasFluids.contains(CO2));
	};
	SUBCASE("four fluids scale 2")
	{
		fourFluidsTest(2, 10);
	}
	// Scale 3 doesn't work due to rounding issues with expectedBlocks.
	SUBCASE("four fluids scale 4")
	{
		fourFluidsTest(4, 21);
	}
	SUBCASE("four fluids scale 5")
	{
		fourFluidsTest(5, 28);
	}
	SUBCASE("four fluids scale 6")
	{
		fourFluidsTest(6, 30);
	}
}
