#include "hasActors.h"
#include "../actor.h"
#include "../block.h"
#include "../area.h"
#include "../simulation.h"
void AreaHasActors::add(Actor& actor)
{
	assert(actor.m_location != nullptr);
	assert(!m_actors.contains(&actor));
	m_actors.insert(&actor);
	m_locationBuckets.add(actor);
	if(!actor.m_location->m_underground)
		m_onSurface.insert(&actor);
	actor.m_canSee.createFacadeIfCanSee();
}
void AreaHasActors::remove(Actor& actor)
{
	m_actors.erase(&actor);
	m_locationBuckets.remove(actor);
	m_visionFacadeBuckets.remove(actor);
	m_onSurface.erase(&actor);
	actor.m_canSee.m_hasVisionFacade.clear();
}
void AreaHasActors::processVisionReadStep()
{
	VisionFacade& visionFacade = m_visionFacadeBuckets.getForStep(m_area.m_simulation.m_step);
	visionFacade.readStep();
}
void AreaHasActors::processVisionWriteStep()
{
	VisionFacade& visionFacade = m_visionFacadeBuckets.getForStep(m_area.m_simulation.m_step);
	visionFacade.writeStep();
}
void AreaHasActors::onChangeAmbiantSurfaceTemperature()
{
	for(Actor* actor : m_onSurface)
		actor->m_needsSafeTemperature.onChange();
}
void AreaHasActors::setUnderground(Actor& actor)
{
	m_onSurface.erase(&actor);
	actor.m_needsSafeTemperature.onChange();
}
void AreaHasActors::setNotUnderground(Actor& actor)
{
	m_onSurface.insert(&actor);
	actor.m_needsSafeTemperature.onChange();
}