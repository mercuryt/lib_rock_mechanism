#pragma once
#include "area.h"
#include "materialType.h"
#include "fluidType.h"
#include "cuboid.h"
#include "blocks/blocks.h"

#include <unordered_set>
#include <cstdint>
#include <string>

namespace areaBuilderUtil
{
	inline void setSolidLayer(Area& area, uint32_t z, const MaterialType& materialType)
	{
		Blocks& blocks = area.getBlocks();
		for(uint32_t x = 0; x != blocks.m_sizeX; ++x)
			for(uint32_t y = 0; y != blocks.m_sizeY; ++y)
				blocks.solid_set(blocks.getIndex({x, y, z}), materialType, false);
	}
	inline void setSolidLayers(Area& area, uint32_t zbegin, uint32_t zend, const MaterialType& materialType)
	{
		for(;zbegin <= zend; ++zbegin)
			setSolidLayer(area, zbegin, materialType);
	}
	inline void setSolidWall(Area& area, BlockIndex start, BlockIndex end, const MaterialType& materialType)
	{
		Blocks& blocks = area.getBlocks();
		Cuboid cuboid(blocks, end, start);
		for(BlockIndex block : cuboid)
			blocks.solid_set(block, materialType, false);
	}
	inline void setSolidWalls(Area& area, uint32_t height, const MaterialType& materialType)
	{	
		Blocks& blocks = area.getBlocks();
		for(uint32_t z = 0; z != height + 1; ++ z)
		{
			for(uint32_t x = 0; x != blocks.m_sizeX; ++x)
			{
				blocks.solid_set(blocks.getIndex({x, 0, z}), materialType, false);
				blocks.solid_set(blocks.getIndex({x, blocks.m_sizeY - 1, z}), materialType, false);
			}
			for(uint32_t y = 0; y != blocks.m_sizeY; ++y)
			{
				blocks.solid_set(blocks.getIndex({0, y, z}), materialType, false);
				blocks.solid_set(blocks.getIndex({blocks.m_sizeX - 1, y, z}), materialType, false);
			}
		}
	}
	inline void makeBuilding(Area& area, Cuboid cuboid, const MaterialType& materialType)
	{
		Blocks& blocks = area.getBlocks();
		Point3D highCoordinates = blocks.getCoordinates(cuboid.m_highest);
		for(uint32_t z = 0; z != area.getBlocks().getZ(cuboid.m_highest) + 2; ++ z)
		{
			for(uint32_t x = 0; x != highCoordinates.x + 1; ++x)
			{
				blocks.solid_set(blocks.getIndex({x, 0, z}), materialType, false);
				blocks.solid_set(blocks.getIndex({x, blocks.m_sizeY - 1, z}), materialType, false);
			}
			for(uint32_t y = 0; y != highCoordinates.y + 1; ++y)
			{
				blocks.solid_set(blocks.getIndex({0, y, z}), materialType, false);
				blocks.solid_set(blocks.getIndex({blocks.m_sizeX - 1, y, z}), materialType, false);
			}
		}
		for(uint32_t x = 0; x != highCoordinates.x + 1; ++x)
			for(uint32_t y = 0; y != highCoordinates.y + 1; ++y)
				blocks.blockFeature_construct(blocks.getIndex({x, y, highCoordinates.z + 1}), BlockFeatureType::floor, materialType);
	}
	inline void setFullFluidCuboid(Area& area, BlockIndex low, BlockIndex high, const FluidType& fluidType)
	{
		Blocks& blocks = area.getBlocks();
		assert(blocks.fluid_getTotalVolume(low) == 0);
		assert(blocks.fluid_canEnterEver(low));
		assert(blocks.fluid_getTotalVolume(high) == 0);
		assert(blocks.fluid_canEnterEver(high));
		Cuboid cuboid(blocks, high, low);
		for(BlockIndex block : cuboid)
		{
			assert(blocks.fluid_getTotalVolume(block) == 0);
			assert(blocks.fluid_canEnterEver(block));
			blocks.fluid_add(block, 100, fluidType);
		}
	}
	inline void validateAllBlockFluids(Area& area)
	{
		Blocks& blocks = area.getBlocks();
		for(uint32_t x = 0; x < blocks.m_sizeX; ++x)
			for(uint32_t y = 0; y < blocks.m_sizeY; ++y)
				for(uint32_t z = 0; z < blocks.m_sizeZ; ++z)
					for([[maybe_unused]] auto& [fluidType, group, volume] : blocks.fluid_getAll(blocks.getIndex({x, y, z})))
						assert(group->m_fluidType == *fluidType);
	}
	// Get one fluid group with the specified type. Return null if there is more then one.
	inline FluidGroup* getFluidGroup(Area& area, const FluidType& fluidType)
	{
		FluidGroup* output = nullptr;
		for(FluidGroup& fluidGroup : area.m_hasFluidGroups.getAll())
			if(fluidGroup.m_fluidType == fluidType)
			{
				if(output != nullptr)
					return nullptr;
				output = &fluidGroup;
			}
		return output;
	}
}
