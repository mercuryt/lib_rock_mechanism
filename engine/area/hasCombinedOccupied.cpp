#include "hasCombinedOccupied.h"
#include "area.h"
#include "../actors/actors.h"
#include "../space/space.h"
void AreaHasCombinedOccupied::record(const ActorIndex& actor, Area& area)
{
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	m_data[faction].maybeInsert(actors.getOccupied(actor));
}
void AreaHasCombinedOccupied::remove(const ActorIndex& actor, Area& area)
{
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	CuboidSet occupied = actors.getOccupied(actor);
	auto condition = [&](const ActorIndex& other) { return actors.getFaction(other) == faction; };
	const auto otherActorFromTheSameFactionIntersectingThisActor = space.actor_queryCuboidsWithCondition(occupied, condition);
	occupied.maybeRemoveAll(otherActorFromTheSameFactionIntersectingThisActor);
	m_data[faction].remove(occupied);
}
bool AreaHasCombinedOccupied::queryAnyEnemyInLineOfSight(const ActorIndex& actor, Area& area) const
{
	const auto hasFactions = area.m_simulation.m_hasFactions;
	Actors& actors = area.getActors();
	const FactionId& faction = actors.getFaction(actor);
	const Point3D& location = actors.getLocation(actor);
	const Sphere visionRange = {actors.getLocation(actor), DistanceFractional::create(actors.vision_getRange(actor))};
	for(const auto& [otherFaction, rtree] : m_data)
		if(hasFactions.isEnemy(faction, otherFaction))
			for(const Cuboid& cuboid : rtree.queryGetLeaves(visionRange))
				// TODO: Batch query?
				for(const Point3D point : cuboid)
					if(rtree.query(ParamaterizedLine(location, point)))
						return true;
	return false;
}