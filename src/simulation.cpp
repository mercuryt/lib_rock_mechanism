#include "simulation.h"
#include "area.h"
#include "threadedTask.h"
void simulation::doStep()
{
	for(Area& area : areas)
		area.readStep(pool);
	threadedTaskEngine::readStep();
	pool.wait_for_tasks();
	for(Area& area : areas)
		area.writeStep();
	threadedTaskEngine::writeStep();
	eventSchedule::execute(step);
	++step;
}
