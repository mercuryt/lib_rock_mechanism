#include "space.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
void Space::actor_recordDynamic(const Point3D& point, const ActorIndex& actor, const CollisionVolume& volume)
{
	Actors& actors = m_area.getActors();
	assert(!actors.isStatic(actor));
	m_actors.updateOrInsertOne(point, [&](SmallSet<ActorIndex>& actors){ actors.insert(actor); });
	m_actorVolume.updateOrInsertOne(point, [&](SmallMap<ActorIndex, CollisionVolume>& actorVolumes){ actorVolumes.insert(actor, volume); });
	m_dynamicVolume.updateAddOne(point, volume);
}
void Space::actor_recordStatic(const Point3D& point, const ActorIndex& actor, const CollisionVolume& volume)
{
	Actors& actors = m_area.getActors();
	assert(actors.isStatic(actor));
	m_actors.updateOrInsertOne(point, [&](SmallSet<ActorIndex>& actors){ actors.insert(actor); });
	m_actorVolume.updateOrInsertOne(point, [&](SmallMap<ActorIndex, CollisionVolume>& actorVolumes){ actorVolumes.insert(actor, volume); });
	m_staticVolume.updateAddOne(point, volume);
}
void Space::actor_eraseStatic(const Point3D& point, const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	assert(actors.isStatic(actor));
	m_actors.updateOne(point, [&](SmallSet<ActorIndex>& actors){ actors.erase(actor); });
	CollisionVolume volume;
	m_actorVolume.updateOne(point, [&](SmallMap<ActorIndex, CollisionVolume>& actorVolumes){
		volume = actorVolumes[actor];
		actorVolumes.erase(actor);
	});
	m_staticVolume.updateSubtractOne(point, volume);
}
void Space::actor_eraseDynamic(const Point3D& point, const ActorIndex& actor)
{
	Actors& actors = m_area.getActors();
	assert(!actors.isStatic(actor));
	m_actors.updateOne(point, [&](SmallSet<ActorIndex>& actors){ actors.erase(actor); });
	CollisionVolume volume;
	m_actorVolume.updateOne(point, [&](SmallMap<ActorIndex, CollisionVolume>& actorVolumes){
		volume = actorVolumes[actor];
		actorVolumes.erase(actor);
	});
	m_dynamicVolume.updateSubtractOne(point, volume);
}
void Space::actor_setTemperature(const Point3D& point, [[maybe_unused]] const Temperature& temperature)
{
	assert(temperature_get(point) == temperature);
	Actors& actors = m_area.getActors();
	auto actorsCopy = m_actors.queryGetOne(point);
	for(const ActorIndex& actor : actorsCopy)
		actors.temperature_onChange(actor);
}
void Space::actor_updateIndex(const Point3D& point, const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	m_actors.updateOne(point, [&](SmallSet<ActorIndex>& data){ data.update(oldIndex, newIndex); });
	m_actorVolume.updateOne(point, [&](SmallMap<ActorIndex, CollisionVolume>& data){ data.updateKey(oldIndex, newIndex); });
}
bool Space::actor_contains(const Point3D& point, const ActorIndex& actor) const
{
	return m_actors.queryGetOne(point).contains(actor);
}
bool Space::actor_empty(const Point3D& point) const
{
	return !m_actors.queryAny(point);
}
const SmallSet<ActorIndex>& Space::actor_getAll(const Point3D& point) const
{
	return m_actors.queryGetOne(point);
}
