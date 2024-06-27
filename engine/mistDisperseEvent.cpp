#include "mistDisperseEvent.h"
#include "config.h"
#include "area.h"
#include "eventSchedule.h"
#include "simulation.h"
#include "fluidType.h"
#include "types.h"

#include <memory>
MistDisperseEvent::MistDisperseEvent(Simulation& simulation, uint32_t delay, const FluidType& ft, BlockIndex b) :
       	ScheduledEvent(simulation, delay), m_fluidType(ft), m_block(b) {}
void MistDisperseEvent::execute(Simulation& simulation, Area* area)
{
	Blocks& blocks = area->getBlocks();
	// Mist does not or cannont exist here anymore, clear and return.
	if(!blocks.fluid_getMist(m_block) || blocks.solid_is(m_block) || blocks.fluid_getTotalVolume(m_block) == Config::maxBlockVolume)
	{
		blocks.fluid_clearMist(m_block);
		return;
	}
	// Check if mist continues to exist here.
	if(continuesToExist(*area))
	{
		// Possibly spread.
		if(blocks.fluid_getMistInverseDistanceToSource(m_block) > 0)
			for(BlockIndex adjacent : blocks.getDirectlyAdjacent(m_block))
				if(adjacent != BLOCK_INDEX_MAX && blocks.fluid_canEnterEver(adjacent) && 
						(
							!blocks.fluid_getMist(adjacent) || 
							blocks.fluid_getMist(adjacent)->density < m_fluidType.density
						)
				)
				{
					blocks.fluid_mistSetFluidTypeAndInverseDistance(adjacent, m_fluidType, blocks.fluid_getMistInverseDistanceToSource(m_block) - 1);
					area->m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(simulation, m_fluidType.mistDuration, m_fluidType, adjacent));
				}
		// Schedule next check.
		area->m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(simulation, m_fluidType.mistDuration, m_fluidType, m_block));	
		return;
	}
	// Mist does not continue to exist here.
	blocks.fluid_clearMist(m_block);
}
bool MistDisperseEvent::continuesToExist(Area& area) const
{
	Blocks& blocks = area.getBlocks();
	if(blocks.fluid_getMist(m_block) == &m_fluidType)
		return true;
	// if adjacent to falling fluid on same z level
	for(BlockIndex adjacent : blocks.getAdjacentOnSameZLevelOnly(m_block))
		// if adjacent to falling fluid.
		if(blocks.fluid_contains(adjacent, m_fluidType))
		{	
			BlockIndex belowAdjacent = blocks.getBlockBelow(adjacent);
			if(belowAdjacent != BLOCK_INDEX_MAX && !blocks.solid_is(belowAdjacent))
				return true;
		}
	for(BlockIndex adjacent : blocks.getDirectlyAdjacent(m_block))
		// if adjacent to block with mist with lower distance to source.
		if(adjacent != BLOCK_INDEX_MAX && 
			blocks.fluid_getMist(adjacent) &&
			blocks.fluid_getMistInverseDistanceToSource(adjacent) > blocks.fluid_getMistInverseDistanceToSource(adjacent) 
		)
			return true;
	return false;
}
void MistDisperseEvent::emplace(Area& area, uint32_t delay, const FluidType& fluidType, BlockIndex block)
{
	std::unique_ptr<ScheduledEvent> event = std::make_unique<MistDisperseEvent>(area.m_simulation, delay, fluidType, block);
	area.m_eventSchedule.schedule(std::move(event));
}
