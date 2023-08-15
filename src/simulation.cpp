#include "simulation.h"
#include "area.h"
#include "threadedTask.h"
void simulation::init(uint32_t s)
{ 
	Actor::data.clear();
	Item::s_globalItems.clear();
	areas.clear();
	eventSchedule::clear();
	step = s; 
}
void simulation::doStep()
{
	for(Area& area : areas)
		area.readStep();
	threadedTaskEngine::readStep();
	pool.wait_for_tasks();
	for(Area& area : areas)
		area.writeStep();
	threadedTaskEngine::writeStep();
	eventSchedule::execute(step);
	++step;
}
