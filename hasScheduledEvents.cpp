ScheduledEvent::ScheduledEvent(uint32_t s) : m_step(s) {}
MoveEvent::MoveEvent(uint32_t s, Actor* a) : ScheduledEvent(s), m_actor(a) { m_actor->m_taskEvent = this; }
void MoveEvent::execute()
{
	Block* block = *(m_actor->m_routeIter);
	if(block->canEnterEver())
	{
		if(block->shapeCanEnterCurrently(m_actor->m_shape))
		{
			m_actor->m_routeIter++;
			block->enter(m_actor);
		}
		if(block == m_actor->m_destination)
		{
			m_actor->m_route.reset();
			m_actor->m_destination = nullptr;
			m_actor->taskComplete();
		}
		else // Schedule move regardless of if we moved already, provided we arn't at destination.
			m_actor->m_location->m_area->scheduleMove(m_actor);
	}
	else // Route is no longer valid, generate new routeRequest with area
		m_actor->m_location->m_area->registerRouteRequest(m_actor);
}
MoveEvent::~MoveEvent()
{
	m_actor->m_taskEvent = nullptr;
}
