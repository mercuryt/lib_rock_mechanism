#pragma once
#include "area.h"
#include "materialType.h"
#include "fluidType.h"
#include "cuboid.h"
#include "blocks/blocks.h"
#include "types.h"

#include <cstdint>
#include <string>

namespace areaBuilderUtil
{
	inline void setSolidLayer(Area& area, const DistanceInBlocks& z, const MaterialTypeId& materialType)
	{
		Blocks& blocks = area.getBlocks();
		for(DistanceInBlocks x = DistanceInBlocks::create(0); x != blocks.m_sizeX; ++x)
			for(DistanceInBlocks y = DistanceInBlocks::create(0); y != blocks.m_sizeY; ++y)
				blocks.solid_set(blocks.getIndex({x, y, z}), materialType, false);
	}
	inline void setSolidLayer(Area& area, uint z, const MaterialTypeId& materialType)
	{
		setSolidLayer(area, DistanceInBlocks::create(z), materialType);
	}
	inline void setSolidLayers(Area& area, DistanceInBlocks zbegin, const DistanceInBlocks& zend, const MaterialTypeId& materialType)
	{
		for(;zbegin <= zend; ++zbegin)
			setSolidLayer(area, zbegin, materialType);
	}
	inline void setSolidLayers(Area& area, uint zbegin, uint zend, const MaterialTypeId& materialType)
	{
		setSolidLayers(area, DistanceInBlocks::create(zbegin), DistanceInBlocks::create(zend), materialType);
	}
	inline void setSolidWall(Area& area, const BlockIndex& start, const BlockIndex& end, const MaterialTypeId& materialType)
	{
		Blocks& blocks = area.getBlocks();
		Cuboid cuboid(blocks, end, start);
		for(BlockIndex block : cuboid)
			blocks.solid_set(block, materialType, false);
	}
	inline void setSolidWall(Area& area, uint start, uint end, const MaterialTypeId& materialType)
	{
		setSolidWall(area, BlockIndex::create(start), BlockIndex::create(end), materialType);
	}
	inline void setSolidWalls(Area& area, const DistanceInBlocks& height, const MaterialTypeId& materialType)
	{	
		Blocks& blocks = area.getBlocks();
		for(DistanceInBlocks z = DistanceInBlocks::create(0); z != height + 1; ++ z)
		{
			for(DistanceInBlocks x = DistanceInBlocks::create(0); x != blocks.m_sizeX; ++x)
			{
				blocks.solid_set(blocks.getIndex(x, DistanceInBlocks::create(0), z), materialType, false);
				blocks.solid_set(blocks.getIndex(x, blocks.m_sizeY - 1, z), materialType, false);
			}
			for(DistanceInBlocks y = DistanceInBlocks::create(0); y != blocks.m_sizeY; ++y)
			{
				blocks.solid_set(blocks.getIndex(DistanceInBlocks::create(0), y, z), materialType, false);
				blocks.solid_set(blocks.getIndex(blocks.m_sizeX - 1, y, z), materialType, false);
			}
		}
	}
	inline void setSolidWalls(Area& area, uint height, const MaterialTypeId& materialType)
	{
		setSolidWalls(area, DistanceInBlocks::create(height), materialType);
	}
	inline void makeBuilding(Area& area, Cuboid cuboid, const MaterialTypeId& materialType)
	{
		Blocks& blocks = area.getBlocks();
		Point3D highCoordinates = blocks.getCoordinates(cuboid.m_highest);
		for(DistanceInBlocks z = DistanceInBlocks::create(0); z != area.getBlocks().getZ(cuboid.m_highest) + 2; ++ z)
		{
			for(DistanceInBlocks x = DistanceInBlocks::create(0); x != highCoordinates.x + 1; ++x)
			{
				blocks.solid_set(blocks.getIndex(x, DistanceInBlocks::create(0), z), materialType, false);
				blocks.solid_set(blocks.getIndex(x, blocks.m_sizeY - 1, z), materialType, false);
			}
			for(DistanceInBlocks y = DistanceInBlocks::create(0); y != highCoordinates.y + 1; ++y)
			{
				blocks.solid_set(blocks.getIndex(DistanceInBlocks::create(0), y, z), materialType, false);
				blocks.solid_set(blocks.getIndex(blocks.m_sizeX - 1, y, z), materialType, false);
			}
		}
		for(DistanceInBlocks x = DistanceInBlocks::create(0); x != highCoordinates.x + 1; ++x)
			for(DistanceInBlocks y = DistanceInBlocks::create(0); y != highCoordinates.y + 1; ++y)
				blocks.blockFeature_construct(blocks.getIndex(x, y, highCoordinates.z + 1), BlockFeatureType::floor, materialType);
	}
	inline void setFullFluidCuboid(Area& area, const BlockIndex& low, const BlockIndex& high, FluidTypeId fluidType)
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
			blocks.fluid_add(block, CollisionVolume::create(100), fluidType);
		}
	}
	inline void setFullFluidCuboid(Area& area, uint low, uint high, FluidTypeId fluidType)
	{
		setFullFluidCuboid(area, BlockIndex::create(low), BlockIndex::create(high), fluidType);
	}
	inline void validateAllBlockFluids(Area& area)
	{
		Blocks& blocks = area.getBlocks();
		for(DistanceInBlocks x = DistanceInBlocks::create(0); x < blocks.m_sizeX; ++x)
			for(DistanceInBlocks y = DistanceInBlocks::create(0); y < blocks.m_sizeY; ++y)
				for(DistanceInBlocks z = DistanceInBlocks::create(0); z < blocks.m_sizeZ; ++z)
					for([[maybe_unused]] auto& [fluidType, group, volume] : blocks.fluid_getAll(blocks.getIndex(x, y, z)))
						assert(group->m_fluidType == fluidType);
	}
	// Get one fluid group with the specified type. Return null if there is more then one.
	inline FluidGroup* getFluidGroup(Area& area, FluidTypeId fluidType)
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
