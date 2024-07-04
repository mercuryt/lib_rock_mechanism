#include "hasActors.h"
#include "../area.h"
#include "../simulation.h"
#include "../actors/actors.h"
#include "types.h"
void AreaHasActors::add(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	assert(!m_actors.contains(actor));
	m_actors.insert(actor);
	actors.vision_createFacadeIfCanSee(actor);
	m_area.m_hasTerrainFacades.maybeRegisterMoveType(actors.getMoveType(actor));
}
void AreaHasActors::remove(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	m_actors.erase(actor);
	m_area.m_locationBuckets.remove(actor);
	m_area.m_visionFacadeBuckets.remove(actor);
	m_onSurface.erase(actor);
	actors.vision_clearFacade(actor);
	// TODO: unregister move type: decrement count of actors using a given facade and maybe clear when 0.
}
void AreaHasActors::onChangeAmbiantSurfaceTemperature()
{
	Actors& actors = m_area.getActors();
	for(ActorIndex actor : m_onSurface)
		actors.temperature_onChange(actor);
}
void AreaHasActors::setUnderground(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	m_onSurface.erase(actor);
	actors.temperature_onChange(actor);
}
void AreaHasActors::setNotUnderground(ActorIndex actor)
{
	Actors& actors = m_area.getActors();
	m_onSurface.insert(actor);
	actors.temperature_onChange(actor);
}
