
#include "space.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../portables.h"

void Space::floating_maybeSink(const CuboidSet& points)
{
	Items& items = m_area.getItems();
	auto condition = [&](const ItemIndex item) { return items.isFloating(item) && !items.canFloatAt(item, items.getLocation(item), items.getFacing(item)); };
	for(const ItemIndex item : m_items.queryGetAllWithCondition(points, condition))
	{
		items.setNotFloating(item);
		const Point3D location = items.getLocation(item);
		if(location.z() == 0)
			continue;
		const Point3D below = location.below();
		if(
			shape_anythingCanEnterEver(below) &&
			shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(below, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getOccupied(item))
		)
			items.fall(item);
	}
}
void Space::floating_maybeFloatUp(const CuboidSet& points)
{
	Items& items = m_area.getItems();
	for(const ItemIndex item : m_items.queryGetAll(points))
	{
		const Facing4& facing = items.getFacing(item);
		const Point3D location = items.getLocation(item);
		FluidTypeId floatingIn;
		if(!items.isFloating(item))
		{
			const FluidTypeId fluidType = items.getFluidTypeCanFloatInAt(item, location, facing);
			if(fluidType.exists())
			{
				//TODO:(optimization) This call is redundant.
				Distance depth = items.floatsInAtDepth(item, fluidType);
				items.setFloating(item, fluidType, depth);
				floatingIn = fluidType;
			}
		}
		if(items.isFloating(item) && fluid_containsVolumeOfEqualOrGreaterDensity(location, floatingIn) == Config::maxPointVolume)
		{
			Point3D current = location;
			Point3D above = location.above();
			// TODO: This traverses multiple z levels at once. Make it move one at a time to mirror falling.
			while(
				above.z() != m_sizeZ - 1 &&
				fluid_containsVolumeOfEqualOrGreaterDensity(current, floatingIn) == Config::maxPointVolume &&
				shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(above, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getOccupied(item)) &&
				shape_canEnterCurrentlyFrom(above, items.getShape(item), above.below(), items.getOccupied(item)) &&
				items.canFloatAt(item, above, facing)
			)
			{
				current = above;
				above = current.above();
			}
			if(current != location)
				items.location_set(item, current, items.getFacing(item));
		}
	}
	// TODO: Dead actors also float up.
}