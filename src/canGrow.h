class AnimalGrowthEvent : ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
	AnimalGrowthEvent(uint32_t step, CanGrow& cg) : ScheduledEventWithPercent(step), m_canGrow(cg) { }
	void execute()
	{
		m_canGrow.complete();
	}
	~AnimalGrowthEvent(){ m_canGrow.m_event.clearPointer(); }
};
class AnimalGrowthUpdateEvent : ScheduledEventWithPercent
{
	CanGrow& m_canGrow;
	AnimalGrowthEvent(uint32_t step, CanGrow& cg) : ScheduledEventWithPercent(step), m_canGrow(cg) { }
	void execute()
	{
		m_canGrow.update();
	}
	~AnimalGrowthEvent(){ m_canGrow.m_updateEvent.clearPointer(); }
};
class CanGrow
{
	Actor& m_actor;
	HasScheduledEvent<AnimalGrowthEvent> m_event;
	HasScheduledEvent<AnimalGrowthUpdateEvent> m_updateEvent;
	uint32_t m_percentGrown;
	CanGrow(Actor& a, uint32_t pg) : m_actor(a), m_percentGrown(pg)
	{ 
		// Note: CanGrow must be initalized after MustEat, MustDrink, and SafeTemperature.
		updateGrowingStatus();
	}
	void updateGrowingStatus()
	{
		if(m_percentGrown != 100 && m_mustEat.ok() && m_mustDrink.ok() && m_safeTemperature.ok())
		{
			if(m_event.empty())
			{
				uint32_t step = ::g_step + (m_percentGrown == 0 ?
						m_actor.m_species.stepsTillFullyGrown :
						((m_actor.m_species.stepsTillFullyGrown * m_percentGrown) / 100u));
				m_event.schedule(step, *this);
				m_updateEvent.schedule(::g_step + Config::statsGrowthUpdateFrequency, *this);
			}
		}
		else if(!m_growthEvent.empty())
		{
			m_percentGrown = growthPercent();
			m_event->unschedule();
			m_updateEvent->unschedule();
		}
	}
	uint32_t growthPercent() const
	{
		uint32_t output = m_percentGrown;
		if(!m_growthEvent.empty())
			output += (m_growthEvent->percentComplete() * m_percentGrown) / 100u;
		return output;
	}
	void update()
	{
		uint32_t percent = growthPercent();
		m_actor.m_attributes.updatePercentGrown(percent);
		if(percent != 100)
			m_updateEvent.schedule(::g_step + Config::statsGrowthUpdateFrequency, *this);
	}
};
