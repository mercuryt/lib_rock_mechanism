#include "mistDisperseEvent.h"
#include "config.h"
#include "area.h"
#include "eventSchedule.h"
#include "simulation.h"
#include "fluidType.h"
#include "types.h"

#include <memory>
MistDisperseEvent::MistDisperseEvent(uint32_t delay, Area& a, const FluidType& ft, BlockIndex b) :
       	ScheduledEvent(a.m_simulation, delay), m_area(a), m_fluidType(ft), m_block(b) {}
void MistDisperseEvent::execute()
{
	Blocks& blocks = m_area.getBlocks();
	// Mist does not or cannont exist here anymore, clear and return.
	if(!blocks.fluid_getMist(m_block) || blocks.solid_is(m_block) || blocks.fluid_getTotalVolume(m_block) == Config::maxBlockVolume)
	{
		blocks.fluid_clearMist(m_block);
		return;
	}
	// Check if mist continues to exist here.
	if(continuesToExist())
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
					m_simulation.m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(m_fluidType.mistDuration, m_area, m_fluidType, adjacent));
				}
		// Schedule next check.
		m_simulation.m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(m_fluidType.mistDuration, m_area, m_fluidType, m_block));	
		return;
	}
	// Mist does not continue to exist here.
	blocks.fluid_clearMist(m_block);
}
bool MistDisperseEvent::continuesToExist() const
{
	Blocks& blocks = m_area.getBlocks();
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
void MistDisperseEvent::emplace(uint32_t delay, Area& area, const FluidType& fluidType, BlockIndex block)
{
	std::unique_ptr<ScheduledEvent> event = std::make_unique<MistDisperseEvent>(delay, area, fluidType, block);
	area.m_simulation.m_eventSchedule.schedule(std::move(event));
}
