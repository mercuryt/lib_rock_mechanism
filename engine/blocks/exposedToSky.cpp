#include "exposedToSky.h"
#include "../blocks/blocks.h"
#include "../area/area.h"
#include "../area/exteriorPortals.h"
void BlocksExposedToSky::initalizeEmpty()
{
	m_data.fill(true);
}
void BlocksExposedToSky::set(Area& area, const BlockIndex& block)
{
	BlockIndex current = block;
	Blocks& blocks = area.getBlocks();
	while(current.exists())
	{
		m_data.set(current);
		if(blocks.solid_is(current))
		{
			const MaterialTypeId& materialType = blocks.solid_get(current);
			if(MaterialType::canMelt(materialType))
				area.m_hasTemperature.addMeltableSolidBlockAboveGround(current);
			break;
		}
		// Check for adjacent which are now portals because they are adjacent to a block exposed to sky.
		for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacentExceptDirectlyAboveAndBelow(current))
			if(
				blocks.temperature_transmits(adjacent) &&
				AreaHasExteriorPortals::isPortal(blocks, adjacent) &&
				!area.m_exteriorPortals.isRecordedAsPortal(adjacent)
			)
				area.m_exteriorPortals.add(area, adjacent);
		// Check if this block is no longer a portal.
		if(area.m_exteriorPortals.isRecordedAsPortal(current))
			area.m_exteriorPortals.remove(area, current);
		blocks.plant_updateGrowingStatus(current);
		current = blocks.getBlockBelow(current);
	}
}
void BlocksExposedToSky::unset(Area& area, const BlockIndex& block)
{
	BlockIndex current = block;
	Blocks& blocks = area.getBlocks();
	while(current.exists() && check(current))
	{
		m_data.maybeUnset(current);
		// Check for adjacent which are no longer portals because they are no longer adjacent to a block exposed to sky.
		for(const BlockIndex& adjacent : blocks.getAdjacentWithEdgeAndCornerAdjacent(current))
			if(area.m_exteriorPortals.isRecordedAsPortal(adjacent) && (!blocks.temperature_transmits(adjacent) || !AreaHasExteriorPortals::isPortal(blocks, adjacent)))
				area.m_exteriorPortals.remove(area, adjacent);
		if(blocks.solid_is(current))
		{
			const MaterialTypeId& materialType = blocks.solid_get(current);
			if(MaterialType::canMelt(materialType))
				area.m_hasTemperature.removeMeltableSolidBlockAboveGround(current);
			break;
		}
		// Check if this block is now a portal.
		if(blocks.temperature_transmits(current) && AreaHasExteriorPortals::isPortal(blocks, current))
			area.m_exteriorPortals.add(area, current);
		blocks.plant_updateGrowingStatus(current);
		current = blocks.getBlockBelow(current);
	}
}