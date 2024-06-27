#include "hasActors.h"
#include "../area.h"
#include "../simulation.h"
#include "types.h"
void AreaHasActors::add(ActorIndex actor)
{
	assert(!m_actors.contains(actor));
	m_actors.insert(actor);
	Actors& actors = m_area.m_actors;
	actors.vision_createFacadeIfCanSee(actor);
	m_area.m_hasTerrainFacades.maybeRegisterMoveType(actors.getMoveType(actor));
}
void AreaHasActors::remove(ActorIndex actor)
{
	m_actors.erase(&actor);
	m_locationBuckets.remove(actor);
	m_visionFacadeBuckets.remove(actor);
	m_onSurface.erase(&actor);
	actor.m_canSee.m_hasVisionFacade.clear();
	// TODO: unregister move type: decrement count of actors using a given facade and maybe clear when 0.
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
	for(ActorIndex actor : m_onSurface)
		actor->m_needsSafeTemperature.onChange();
}
void AreaHasActors::setUnderground(ActorIndex actor)
{
	m_onSurface.erase(&actor);
	actor.m_needsSafeTemperature.onChange();
}
void AreaHasActors::setNotUnderground(ActorIndex actor)
{
	m_onSurface.insert(&actor);
	actor.m_needsSafeTemperature.onChange();
}
