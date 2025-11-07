#pragma once
#include "area/area.h"
#include "definitions/materialType.h"
#include "fluidType.h"
#include "geometry/cuboid.h"
#include "space/space.h"
#include "numericTypes/types.h"

#include <cstdint>
#include <string>

namespace areaBuilderUtil
{
	inline void setSolidLayer(Area& area, const Distance& z, const MaterialTypeId& materialType)
	{
		Space& space = area.getSpace();
		Cuboid cuboid(
			{space.m_sizeX - 1, space.m_sizeY - 1, z},
			{Distance::create(0), Distance::create(0), z}
		);
		space.solid_setCuboid(cuboid, materialType, false);
	}
	inline void setSolidLayer(Area& area, uint z, const MaterialTypeId& materialType)
	{
		setSolidLayer(area, Distance::create(z), materialType);
	}
	inline void setSolidLayers(Area& area, Distance zbegin, const Distance& zend, const MaterialTypeId& materialType)
	{
		for(;zbegin <= zend; ++zbegin)
			setSolidLayer(area, zbegin, materialType);
		area.getSpace().solid_prepare();
	}
	inline void setSolidLayers(Area& area, uint zbegin, uint zend, const MaterialTypeId& materialType)
	{
		setSolidLayers(area, Distance::create(zbegin), Distance::create(zend), materialType);
	}
	inline void setSolidWall(Area& area, const Point3D& start, const Point3D& end, const MaterialTypeId& materialType)
	{
		Space& space = area.getSpace();
		Cuboid cuboid(end, start);
		space.solid_setCuboid(cuboid, materialType, false);
		space.solid_prepare();
	}
	inline void setSolidWalls(Area& area, const Distance& height, const MaterialTypeId& materialType)
	{
		Space& space = area.getSpace();
		static constexpr Distance zero = Distance::create(0);
		Cuboid cuboid({space.m_sizeX - 1, zero, height}, {zero, zero, zero});
		space.solid_setCuboid(cuboid, materialType, false);
		cuboid = {{space.m_sizeX - 1, space.m_sizeY - 1, height}, {zero, space.m_sizeY - 1, zero}};
		space.solid_setCuboid(cuboid, materialType, false);
		cuboid = {{zero, space.m_sizeY - 1, height}, {zero, zero, zero}};
		space.solid_setCuboid(cuboid, materialType, false);
		cuboid = {{space.m_sizeX - 1, space.m_sizeY - 1, height}, {space.m_sizeX - 1, zero, zero}};
		space.solid_setCuboid(cuboid, materialType, false);
		space.solid_prepare();
	}
	inline void setSolidWalls(Area& area, uint height, const MaterialTypeId& materialType)
	{
		setSolidWalls(area, Distance::create(height), materialType);
	}
	inline void makeBuilding(Area& area, Cuboid cuboid, const MaterialTypeId& materialType)
	{
		Space& space = area.getSpace();
		Point3D highCoordinates = cuboid.m_high;
		for(Distance z = Distance::create(0); z != cuboid.m_high.z() + 2; ++ z)
		{
			for(Distance x = Distance::create(0); x != highCoordinates.x() + 1; ++x)
			{
				space.solid_set({x, Distance::create(0), z}, materialType, false);
				space.solid_set({x, space.m_sizeY - 1, z}, materialType, false);
			}
			for(Distance y = Distance::create(0); y != highCoordinates.y() + 1; ++y)
			{
				space.solid_set({Distance::create(0), y, z}, materialType, false);
				space.solid_set({space.m_sizeX - 1, y, z}, materialType, false);
			}
		}
		for(Distance x = Distance::create(0); x != highCoordinates.x() + 1; ++x)
			for(Distance y = Distance::create(0); y != highCoordinates.y() + 1; ++y)
				space.pointFeature_construct({x, y, highCoordinates.z() + 1}, PointFeatureTypeId::Floor, materialType);
	}
	// TODO: low and high should be inverted.
	inline void setFullFluidCuboid(Area& area, const Point3D& low, const Point3D& high, FluidTypeId fluidType)
	{
		Space& space = area.getSpace();
		assert(space.fluid_getTotalVolume(low) == 0);
		assert(space.fluid_canEnterEver(low));
		assert(space.fluid_getTotalVolume(high) == 0);
		assert(space.fluid_canEnterEver(high));
		Cuboid cuboid(high, low);
		for(const Point3D& point : cuboid)
		{
			assert(space.fluid_getTotalVolume(point) == 0);
			assert(space.fluid_canEnterEver(point));
			space.fluid_add(point, CollisionVolume::create(100), fluidType);
		}
		space.prepareRtrees();
	}
	inline void validateAllPointFluids(Area& area)
	{
		Space& space = area.getSpace();
		Cuboid cuboid = space.boundry();
		for(const Point3D& point : cuboid)
			for([[maybe_unused]] auto& [group, fluidType, volume] : space.fluid_getAll(point))
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
