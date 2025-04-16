
 #include "blocks.h"
 #include "../area/area.h"
 #include "../items/items.h"
 #include "../portables.hpp"

void Blocks::floating_maybeSink(const BlockIndex& index)
{
	Items& items = m_area.getItems();
	for(const ItemIndex& item : item_getAll(index))
	{
		const BlockIndex& location = items.getLocation(item);
		if(items.isFloating(item) && !items.canFloatAt(item, location))
		{
			items.setNotFloating(item);
			const BlockIndex& below = getBlockBelow(location);
			if(below.exists() && shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(below, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getBlocks(item)))
				items.fall(item);
		}
	}
}
void Blocks::floating_maybeFloatUp(const BlockIndex& index)
{
	Items& items = m_area.getItems();
	auto copy = item_getAll(index);
	for(const ItemIndex& item : copy)
	{
		const BlockIndex& location = items.getLocation(item);
		if(!items.isFloating(item))
		{
			const FluidTypeId& fluidType = items.getFluidTypeCanFloatInAt(item, location);
			items.setFloating(item, fluidType);
		}
		if(items.isFloating(item))
		{
			BlockIndex above = location;
			while(above.exists() && shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(above, items.getShape(item), items.getMoveType(item), items.getFacing(item), items.getBlocks(item)) && items.canFloatAt(item, above))
				above = getBlockAbove(above);
			if(above != location)
				items.setLocation(item, above);
		}
	}
}