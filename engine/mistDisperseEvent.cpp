#include "mistDisperseEvent.h"
#include "config.h"
#include "area/area.h"
#include "eventSchedule.h"
#include "simulation/simulation.h"
#include "fluidType.h"
#include "types.h"
#include "blocks/blocks.h"

#include <memory>
MistDisperseEvent::MistDisperseEvent(const Step& delay, Simulation& simulation, const FluidTypeId& ft, const BlockIndex& b) :
       	ScheduledEvent(simulation, delay), m_fluidType(ft), m_block(b) {}
void MistDisperseEvent::execute(Simulation& simulation, Area* area)
{
	Blocks& blocks = area->getBlocks();
	// Mist does not or cannont exist here anymore, clear and return.
	if(blocks.fluid_getMist(m_block).empty() || blocks.solid_is(m_block) || blocks.fluid_getTotalVolume(m_block) == Config::maxBlockVolume)
	{
		blocks.fluid_clearMist(m_block);
		return;
	}
	// Check if mist continues to exist here.
	if(continuesToExist(*area))
	{
		// Possibly spread.
		if(blocks.fluid_getMistInverseDistanceToSource(m_block) > 0)
			for(const BlockIndex& adjacent : blocks.getDirectlyAdjacent(m_block))
				if(blocks.fluid_canEnterEver(adjacent) &&
						(
							blocks.fluid_getMist(adjacent).empty() ||
							FluidType::getDensity(blocks.fluid_getMist(adjacent)) < FluidType::getDensity(m_fluidType)
						)
				)
				{
					blocks.fluid_mistSetFluidTypeAndInverseDistance(adjacent, m_fluidType, blocks.fluid_getMistInverseDistanceToSource(m_block) - 1);
					area->m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(FluidType::getMistDuration(m_fluidType), simulation, m_fluidType, adjacent));
				}
		// Schedule next check.
		area->m_eventSchedule.schedule(std::make_unique<MistDisperseEvent>(FluidType::getMistDuration(m_fluidType), simulation, m_fluidType, m_block));
		return;
	}
	// Mist does not continue to exist here.
	blocks.fluid_clearMist(m_block);
}
bool MistDisperseEvent::continuesToExist(Area& area) const
{
	Blocks& blocks = area.getBlocks();
	if(blocks.fluid_getMist(m_block) == m_fluidType)
		return true;
	// if adjacent to falling fluid on same z level
	for(BlockIndex adjacent : blocks.getAdjacentOnSameZLevelOnly(m_block))
		// if adjacent to falling fluid.
		if(blocks.fluid_contains(adjacent, m_fluidType))
		{
			BlockIndex belowAdjacent = blocks.getBlockBelow(adjacent);
			if(belowAdjacent.exists() && !blocks.solid_is(belowAdjacent))
				return true;
		}
	for(const BlockIndex& adjacent : blocks.getDirectlyAdjacent(m_block))
		// if adjacent to block with mist with lower distance to source.
		if(blocks.fluid_getMist(adjacent).exists() &&
			blocks.fluid_getMistInverseDistanceToSource(adjacent) > blocks.fluid_getMistInverseDistanceToSource(adjacent)
		)
			return true;
	return false;
}
void MistDisperseEvent::emplace(Area& area, const Step& delay, const FluidTypeId& fluidType, const BlockIndex& block)
{
	std::unique_ptr<ScheduledEvent> event = std::make_unique<MistDisperseEvent>(delay, area.m_simulation, fluidType, block);
	area.m_eventSchedule.schedule(std::move(event));
}
