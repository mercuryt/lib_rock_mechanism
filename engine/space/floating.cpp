
#include "space.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../portables.hpp"

void Space::floating_maybeSink(const Point3D& index)
{
	Items& items = m_area.getItems();
	for(const ItemIndex& item : item_getAll(index))
	{
		const Point3D& location = items.getLocation(item);
		if(items.isFloating(item) && !items.canFloatAt(item, location))
		{
			items.setNotFloating(item);
			if(location.z() == 0)
				continue;
			const Point3D& below = location.below();
			if(shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(below, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getOccupied(item)))
				items.fall(item);
		}
	}
}
void Space::floating_maybeFloatUp(const Point3D& index)
{
	Items& items = m_area.getItems();
	auto copy = item_getAll(index);
	for(const ItemIndex& item : copy)
	{
		const Point3D& location = items.getLocation(item);
		if(!items.isFloating(item))
		{
			const FluidTypeId& fluidType = items.getFluidTypeCanFloatInAt(item, location);
			items.setFloating(item, fluidType);
		}
		if(items.isFloating(item))
		{
			Point3D above = location;
			while(above.exists() && shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(above, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getOccupied(item)) && items.canFloatAt(item, above))
				above = above.above();
			if(above != location)
				items.location_set(item, above, items.getFacing(item));
		}
	}
	// TODO: Dead actors also float up.
}