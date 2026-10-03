#include "actors.h"
#include "../psycology/psycologyData.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"
#include "../vision/visionRequests.h"
#include "../area/area.h"
#include "../space/space.h"
bool Actors::vision_canSeeAnything(ActorIndex index) const
{
	return isAlive(index) && sleep_isAwake(index);
}
bool Actors::vision_canSeeEnemy(ActorIndex index) const
{
	for(const ActorReference actor : m_canSee[index])
		if(isEnemy(index, actor.getIndex(m_referenceData)))
			return true;
	return false;
}
bool Actors::vision_canSeeActor(ActorIndex index, ActorIndex actor) const
{
	ActorReference ref = m_area.getActors().getReference(actor);
	return m_canSee[index].contains(ref);
}
void Actors::vision_createRequestIfCanSee(ActorIndex index)
{
	if(vision_canSeeAnything(index))
		m_area.m_visionRequests.create(m_area.getActors().getReference(index));
}
void Actors::vision_clearRequestIfExists(ActorIndex index)
{
	m_area.m_visionRequests.cancelIfExists(m_area.getActors().getReference(index));
}
void Actors::vision_setCanSee(ActorIndex index, const ActorReference other)
{
	m_canSee[index].maybeInsert(other);
}
void Actors::vision_setCanBeSeenBy(ActorIndex index, const ActorReference other)
{
	m_canBeSeenBy[index].maybeInsert(other);
}
void Actors::vision_setNoLongerCanSee(ActorIndex index, const ActorReference other)
{
	m_canSee[index].maybeErase(other);
}
void Actors::vision_setNoLongerCanBeSeenBy(ActorIndex index, const ActorReference other)
{
	m_canBeSeenBy[index].maybeErase(other);
}
void Actors::vision_setCanSee(ActorIndex index, SmallSet<ActorReference>&& others)
{
	m_canSee[index] = std::move(others);
}
void Actors::vision_setCanBeSeenBy(ActorIndex index, SmallSet<ActorReference>&& others)
{
	m_canBeSeenBy[index] = std::move(others);
}
void Actors::vision_clearCanSee(ActorIndex index)
{
	ActorReference ref = m_area.getActors().getReference(index);
	for(ActorReference other : m_canSee[index])
		m_canBeSeenBy[other.getIndex(m_referenceData)].erase(ref);
	m_canSee[index].clear();
}
void Actors::vision_maybeUpdateRange(ActorIndex index, const Distance range)
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
void Actors::vision_maybeUpdateLocation(ActorIndex index, const Point3D location)
{
	if(vision_canSeeAnything(index))
	{
		ActorReference ref = m_area.getActors().getReference(index);
		bool exists = m_area.m_visionRequests.maybeUpdateLocation(ref, location);
		if(!exists)
			m_area.m_visionRequests.create(ref);
	}
}