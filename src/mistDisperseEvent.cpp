#include "mistDisperseEvent.h"
#include "config.h"
#include "block.h"
#include "area.h"
#include "eventSchedule.h"
#include <memory>
void MistDisperseEvent::execute()
{
	// Mist does not or cannont exist here anymore, clear and return.
	if(m_block.m_mist == nullptr || m_block.isSolid() || m_block.m_totalFluidVolume == Config::maxBlockVolume)
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
				if(adjacent->fluidCanEnterEver() && (adjacent->m_mist == nullptr || adjacent->m_mist->density < m_fluidType.density)
				  )
				{
					adjacent->m_mist = &m_fluidType;
					adjacent->m_mistInverseDistanceFromSource = m_block.m_mistInverseDistanceFromSource - 1;
					eventSchedule::schedule(std::make_unique<MistDisperseEvent>(m_fluidType.mistDuration, m_fluidType, *adjacent));
				}
		// Schedule next check.
		eventSchedule::schedule(std::make_unique<MistDisperseEvent>(m_fluidType.mistDuration, m_fluidType, m_block));	
		return;
	}
	// Mist does not continue to exist here.
	m_block.m_mist = nullptr;
	m_block.m_mistInverseDistanceFromSource = UINT32_MAX;
}
bool MistDisperseEvent::continuesToExist() const
{
	if(m_block.m_mistSource == &m_fluidType)
		return true;
	// if adjacent to falling fluid on same z level
	for(Block* adjacent : m_block.getAdjacentOnSameZLevelOnly())
		// if adjacent to falling fluid.
		if(adjacent->m_fluids.contains(&m_fluidType) && !adjacent->m_adjacents[0]->isSolid())
			return true;
	for(Block* adjacent : m_block.m_adjacentsVector)
		// if adjacent to block with mist with lower distance to source.
		if(adjacent->m_mist == &m_fluidType && adjacent->m_mistInverseDistanceFromSource > m_block.m_mistInverseDistanceFromSource)
			return true;
	return false;
}
