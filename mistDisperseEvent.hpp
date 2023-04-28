template<class DerivedBlock, class DerivedActor, class DerivedArea>
MistDisperseEvent<DerivedBlock, DerivedActor, DerivedArea>::MistDisperseEvent(uint32_t s, const FluidType* ft, DerivedBlock& b) :
       	ScheduledEvent(s), m_fluidType(ft), m_block(b) {}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
void MistDisperseEvent<DerivedBlock, DerivedActor, DerivedArea>::execute()
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
			for(DerivedBlock* adjacent : m_block.m_adjacentsVector)
				if(adjacent->fluidCanEnterEver() and adjacent->fluidCanEnterEver(m_fluidType) and
						(adjacent->m_mist == nullptr or adjacent->m_mist->density < m_fluidType->density)
				  )
				{
					adjacent->m_mist = m_fluidType;
					adjacent->m_mistInverseDistanceFromSource = m_block.m_mistInverseDistanceFromSource - 1;
					auto event = std::make_unique<MistDisperseEvent<DerivedBlock, DerivedActor, DerivedArea>>( s_step + m_fluidType->mistDuration, m_fluidType, static_cast<DerivedBlock&>(*adjacent));
					m_block.m_area->m_eventSchedule.schedule(std::move(event));
				}
		// Schedule next check.
		auto event = std::make_unique<MistDisperseEvent<DerivedBlock, DerivedActor, DerivedArea>>( s_step + m_fluidType->mistDuration, m_fluidType, static_cast<DerivedBlock&>(m_block));
		m_block.m_area->m_eventSchedule.schedule(std::move(event));
		return;
	}
	// Mist does not continue to exist here.
	m_block.m_mist = nullptr;
	m_block.m_mistInverseDistanceFromSource = UINT32_MAX;
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
bool MistDisperseEvent<DerivedBlock, DerivedActor, DerivedArea>::continuesToExist() const
{
	if(m_block.m_mistSource == m_fluidType)
		return true;
	// if adjacent to falling fluid on same z level
	for(DerivedBlock* adjacent : m_block.getAdjacentOnSameZLevelOnly())
		// if adjacent to falling fluid.
		if(adjacent->m_fluids.contains(m_fluidType) and not adjacent->m_adjacents[0]->isSolid())
			return true;
	for(DerivedBlock* adjacent : m_block.m_adjacentsVector)
		// if adjacent to block with mist with lower distance to source.
		if(adjacent->m_mist == m_fluidType and adjacent->m_mistInverseDistanceFromSource > m_block.m_mistInverseDistanceFromSource)
			return true;
	return false;
}
