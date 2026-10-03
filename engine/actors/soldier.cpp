#include "actors.h"
#include "../area/area.h"
#include "../config/psycology.h"

// When a soldier is demobilized, loses conciousness, dies, or leaves area.
void Actors::soldier_removeFromMaliceMap(ActorIndex index)
{
	m_area.m_hasSoldiers.remove(m_area, index);
	// No need to clear maliceZone, it get's overrwritten before each use.
}
// When a soldier is drafted, regains conciousness, or arrives at area.
void Actors::soldier_recordInMaliceMap(ActorIndex index)
{
	m_area.m_hasSoldiers.add(m_area, index);
}
void Actors::soldier_mobilize(ActorIndex index)
{
	m_soldier[index].isDrafted = true;
	soldier_recordInMaliceMap(index);
}
void Actors::soldier_demobilize(ActorIndex index)
{
	soldier_removeFromMaliceMap(index);
	m_soldier[index].isDrafted = false;
}
void Actors::soldier_onSetLocation(ActorIndex index)
{
	soldier_recordInMaliceMap(index);
}
void Actors::soldier_setSquad(ActorIndex index, const SquadIndex squad)
{
	m_soldier[index].squad = squad;
}
bool Actors::soldier_is(ActorIndex index) const { return m_soldier[index].squad.exists(); }
CuboidSet& Actors::soldier_getRecordedMalicePoints(ActorIndex index) { return m_soldier[index].maliceZone; }