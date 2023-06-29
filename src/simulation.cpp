void Simulation::step()
{
	for(Area& area : m_areas)
		area.readStep(m_pool);
	m_threadedTaskEngine.readStep(m_pool);
	m_pool.wait_for_tasks();
	for(Area& area : m_areas)
		area.writeStep();
	m_threadedTaskEngine.writeStep();
	m_eventSchedule.execute(m_step);
	++m_step;
}
