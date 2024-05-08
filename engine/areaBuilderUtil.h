#pragma once
#include "area.h"
#include "block.h"
#include "materialType.h"
#include "fluidType.h"
#include "cuboid.h"

#include <unordered_set>
#include <cstdint>
#include <string>

namespace areaBuilderUtil
{
	inline void setSolidLayer(Area& area, uint32_t z, const MaterialType& materialType)
	{
		for(uint32_t x = 0; x != area.m_sizeX; ++x)
			for(uint32_t y = 0; y != area.m_sizeY; ++y)
				area.getBlock(x, y, z).setSolid(materialType);
	}
	inline void setSolidLayers(Area& area, uint32_t zbegin, uint32_t zend, const MaterialType& materialType)
	{
		for(;zbegin <= zend; ++zbegin)
			setSolidLayer(area, zbegin, materialType);
	}
	inline void setSolidWall(Block& start, Block& end, const MaterialType& materialType)
	{
		Cuboid cuboid(end, start);
		for(Block& block : cuboid)
			block.setSolid(materialType);
	}
	inline void setSolidWalls(Area& area, uint32_t height, const MaterialType& materialType)
	{	
		for(uint32_t z = 0; z != height + 1; ++ z)
		{
			for(uint32_t x = 0; x != area.m_sizeX; ++x)
			{
				area.getBlock(x, 0, z).setSolid(materialType);
				area.getBlock(x, area.m_sizeY - 1, z).setSolid(materialType);
			}
			for(uint32_t y = 0; y != area.m_sizeY; ++y)
			{
				area.getBlock(0, y, z).setSolid(materialType);
				area.getBlock(area.m_sizeX - 1, y, z).setSolid(materialType);
			}
		}
	}
	inline void makeBuilding(Area& area, Cuboid cuboid, const MaterialType& materialType)
	{
		for(uint32_t z = 0; z != cuboid.m_highest->m_z + 2; ++ z)
		{
			for(uint32_t x = 0; x != cuboid.m_highest->m_x + 1; ++x)
			{
				area.getBlock(x, 0, z).setSolid(materialType);
				area.getBlock(x, area.m_sizeY - 1, z).setSolid(materialType);
			}
			for(uint32_t y = 0; y != cuboid.m_highest->m_y + 1; ++y)
			{
				area.getBlock(0, y, z).setSolid(materialType);
				area.getBlock(area.m_sizeX - 1, y, z).setSolid(materialType);
			}
		}
		for(uint32_t x = 0; x != cuboid.m_highest->m_x + 1; ++x)
			for(uint32_t y = 0; y != cuboid.m_highest->m_y + 1; ++y)
				area.getBlock(x, y, cuboid.m_highest->m_z + 1).m_hasBlockFeatures.construct(BlockFeatureType::floor, materialType);
	}
	inline void setFullFluidCuboid(Block& low, Block& high, const FluidType& fluidType)
	{
		assert(low.m_hasFluids.getTotalVolume() == 0);
		assert(low.m_hasFluids.fluidCanEnterEver());
		assert(high.m_hasFluids.getTotalVolume() == 0);
		assert(high.m_hasFluids.fluidCanEnterEver());
		Cuboid cuboid(high, low);
		for(Block& block : cuboid)
		{
			assert(block.m_hasFluids.getTotalVolume() == 0);
			assert(block.m_hasFluids.fluidCanEnterEver());
			block.m_hasFluids.addFluid(100, fluidType);
		}
	}
	inline void validateAllBlockFluids(Area& area)
	{
		for(uint32_t x = 0; x < area.m_sizeX; ++x)
			for(uint32_t y = 0; y < area.m_sizeY; ++y)
				for(uint32_t z = 0; z < area.m_sizeZ; ++z)
					for([[maybe_unused]] auto& [fluidType, pair] : area.getBlock(x, y, z).m_hasFluids.getAll())
						assert(pair.second->m_fluidType == *fluidType);
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
