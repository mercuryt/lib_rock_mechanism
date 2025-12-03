
#include "space.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../portables.h"

void Space::floating_maybeSink(const CuboidSet& points)
{
	Items& items = m_area.getItems();
	auto condition = [&](const ItemIndex& item) { return items.isFloating(item) && !items.canFloatAt(item, items.getLocation(item), items.getFacing(item)); };
	for(const ItemIndex& item : m_items.queryGetAllWithCondition(points, condition))
	{
		items.setNotFloating(item);
		const Point3D location = items.getLocation(item);
		if(location.z() == 0)
			continue;
		const Point3D& below = location.below();
		if(shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(below, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getOccupied(item)))
			items.fall(item);
	}
}
void Space::floating_maybeFloatUp(const CuboidSet& points)
{
	Items& items = m_area.getItems();
	for(const ItemIndex& item : m_items.queryGetAll(points))
	{
		const Facing4& facing = items.getFacing(item);
		const Point3D& location = items.getLocation(item);
		if(!items.isFloating(item))
		{
			const FluidTypeId& fluidType = items.getFluidTypeCanFloatInAt(item, location, facing);
			if(fluidType.exists())
				items.setFloating(item, fluidType);
		}
		if(items.isFloating(item))
		{
			Point3D above = location;
			while(above.exists() && shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(above, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getOccupied(item)) && items.canFloatAt(item, above, facing))
				above = above.above();
			if(above != location)
				items.location_set(item, above, items.getFacing(item));
		}
	}
	// TODO: Dead actors also float up.
}