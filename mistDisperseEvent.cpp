MistDisperseEvent::MistDisperseEvent(uint32_t s, const FluidType* ft, Block& b) :
       	ScheduledEvent(s), m_fluidType(ft), m_block(b) {}
void MistDisperseEvent::execute()
{
	// Mist does not or cannont exist here anymore, clear and return.
	if(m_block.m_mist == nullptr or m_block.isSolid() or m_block.m_totalFluidVolume == s_maxBlockVolume)
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
						(adjacent->m_mist == nullptr or adjacent->m_mist->density < m_fluidType->density)
				  )
				{
					adjacent->m_mist = m_fluidType;
					adjacent->m_mistInverseDistanceFromSource = m_block.m_mistInverseDistanceFromSource - 1;
					MistDisperseEvent* event = new MistDisperseEvent(s_step + m_fluidType->mistDuration, m_fluidType, *adjacent);
					m_block.m_area->scheduleEvent(event);
				}
		// Schedule next check.
		MistDisperseEvent* event = new MistDisperseEvent(s_step + m_fluidType->mistDuration, m_fluidType, m_block);
		m_block.m_area->scheduleEvent(event);
		return;
	}
	// Mist does not continue to exist here.
	m_block.m_mist = nullptr;
	m_block.m_mistInverseDistanceFromSource = 0;
}

bool MistDisperseEvent::continuesToExist() const
{
	if(m_block.m_mistSource == m_fluidType)
		return true;
	// if adjacent to falling fluid on same z level
	for(Block* adjacent : m_block.getAdjacentOnSameZLevelOnly())
		// if adjacent to falling fluid.
		if(adjacent->m_fluids.contains(m_fluidType) and not adjacent->m_adjacents[0]->isSolid())
			return true;
	for(Block* adjacent : m_block.m_adjacentsVector)
		// if adjacent to block with mist with lower distance to source.
		if(adjacent->m_mist == m_fluidType and adjacent->m_mistInverseDistanceFromSource > m_block.m_mistInverseDistanceFromSource)
			return true;
	return false;
}
