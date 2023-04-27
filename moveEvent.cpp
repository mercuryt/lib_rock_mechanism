MoveEvent::MoveEvent(uint32_t s, ACTOR& a) : ScheduledEvent(s), m_actor(a) { m_actor.m_taskEvent = this; }
void MoveEvent::execute()
{
	BLOCK& block = **m_actor.m_routeIter;
	if(block.anyoneCanEnterEver() && block.canEnterEver(m_actor))
	{
		if(block.actorCanEnterCurrently(m_actor))
		{
			m_actor.m_routeIter++;
			m_actor.setLocation(&block);
		}
		else
		{
			m_actor.m_taskDelayCount++;
			// Request detour with canEnterCurrently pathing if waiting too long.
			if(m_actor.m_taskDelayCount == s_moveTryAttemptsBeforeDetour)
			{
				m_actor.m_location->m_area->registerRouteRequest(m_actor, true);
				return;
			}
		}
		if(&block == m_actor.m_destination)
		{
			m_actor.m_route.reset();
			m_actor.m_destination = nullptr;
			m_actor.taskComplete();
		}
		else // Schedule move regardless of if we moved already, provided we are not at destination.
			m_actor.m_location->m_area->scheduleMove(m_actor);
	}
	else // Route is no longer valid, generate new routeRequest with area
		m_actor.m_location->m_area->registerRouteRequest(m_actor);
}
MoveEvent::~MoveEvent()
{
	m_actor.m_taskEvent = nullptr;
}
