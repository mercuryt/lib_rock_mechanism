#include "actors.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"
#include "../vision/visionRequests.h"
#include "../area/area.h"
#include "../space/space.h"
bool Actors::vision_canSeeAnything(const ActorIndex& index) const
{
	return isAlive(index) && sleep_isAwake(index);
}
bool Actors::vision_canSeeActor(const ActorIndex& index, const ActorIndex& actor) const
{
	ActorReference ref = m_area.getActors().getReference(actor);
	return m_canSee[index].contains(ref);
}
void Actors::vision_createRequestIfCanSee(const ActorIndex& index)
{
	if(vision_canSeeAnything(index))
		m_area.m_visionRequests.create(m_area.getActors().getReference(index));
}
void Actors::vision_clearRequestIfExists(const ActorIndex& index)
{
	m_area.m_visionRequests.cancelIfExists(m_area.getActors().getReference(index));
}
void Actors::vision_clearCanSee(const ActorIndex& index)
{
	ActorReference ref = m_area.getActors().getReference(index);
	for(ActorReference other : m_canSee[index])
		m_canBeSeenBy[other.getIndex(m_referenceData)].erase(ref);
	m_canSee[index].clear();
}
void Actors::vision_maybeUpdateCuboid(const ActorIndex& index, const VisionCuboidId& newCuboid)
{
	if(vision_canSeeAnything(index))
		m_area.m_visionRequests.maybeUpdateCuboid(getReference(index), newCuboid);
}
void Actors::vision_maybeUpdateRange(const ActorIndex& index, const Distance& range)
{
	if(vision_canSeeAnything(index) && hasLocation(index))
	{
		ActorReference ref = m_area.getActors().getReference(index);
		bool exists = m_area.m_visionRequests.maybeUpdateRange(ref, range);
		if(!exists)
			m_area.m_visionRequests.create(ref);
		m_area.m_octTree.updateRange(ref, m_area.getActors().getLocation(index), range.squared());
	}
}
void Actors::vision_maybeUpdateLocation(const ActorIndex& index, const Point3D& location)
{
	if(vision_canSeeAnything(index))
	{
		ActorReference ref = m_area.getActors().getReference(index);
		bool exists = m_area.m_visionRequests.maybeUpdateLocation(ref, location);
		if(!exists)
			m_area.m_visionRequests.create(ref);
	}
}