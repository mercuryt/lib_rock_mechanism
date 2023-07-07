void simulation::step()
{
	for(Area& area : areas)
		area.readStep(pool);
	threadedTaskEngine.readStep(pool);
	pool.wait_for_tasks();
	for(Area& area : areas)
		area.writeStep();
	threadedTaskEngine.writeStep();
	eventSchedule.execute(step);
	++step;
}
