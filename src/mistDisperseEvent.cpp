#include "mistDisperseEvent.h"
#include <memory>
void MistDisperseEvent::execute()
{
	// Mist does not or cannont exist here anymore, clear and return.
	if(m_block.m_mist == nullptr or m_block.isSolid() or m_block.m_totalFluidVolume == Config::maxBlockVolume)
	{
		m_block.m_mist = nullptr;
		m_block.m_mistInverseDistanceFromSource = 0;
		return;
	}
	// Check if mist continues to exist here.
	if(continuesToExist())
	{
		// Possibly spread.
		if(m_block.m_mistInverseDistanceFromSource > 0)
			for(Block* adjacent : m_block.m_adjacentsVector)
				if(adjacent->fluidCanEnterEver() and adjacent->fluidCanEnterEver(m_fluidType) and
						(adjacent->m_mist == nullptr or adjacent->m_mist->density < m_fluidType.density)
				  )
				{
					adjacent->m_mist = &m_fluidType;
					adjacent->m_mistInverseDistanceFromSource = m_block.m_mistInverseDistanceFromSource - 1;
					emplace(m_block.m_area->m_eventSchedule, m_fluidType.mistDuration, m_fluidType, static_cast<Block&>(*adjacent));
				}
		// Schedule next check.
		emplace(m_block.m_area->m_eventSchedule, m_fluidType.mistDuration, m_fluidType, static_cast<Block&>(m_block));
		return;
	}
	// Mist does not continue to exist here.
	m_block.m_mist = nullptr;
	m_block.m_mistInverseDistanceFromSource = UINT32_MAX;
}
bool MistDisperseEvent::continuesToExist() const
{
	if(m_block.m_mistSource == m_fluidType)
		return true;
	// if adjacent to falling fluid on same z level
	for(Block* adjacent : m_block.getAdjacentOnSameZLevelOnly())
		// if adjacent to falling fluid.
		if(adjacent->m_fluids.contains(&m_fluidType) and not adjacent->m_adjacents[0]->isSolid())
			return true;
	for(Block* adjacent : m_block.m_adjacentsVector)
		// if adjacent to block with mist with lower distance to source.
		if(adjacent->m_mist == m_fluidType and adjacent->m_mistInverseDistanceFromSource > m_block.m_mistInverseDistanceFromSource)
			return true;
	return false;
}
void MistDisperseEvent::emplace(uint32_t delay, const FluidType& fluidType, Block& block)
{
	eventSchedule::scedule(std::make_unique<MistDisperseEvent>(delay, fluidType, block));
}
