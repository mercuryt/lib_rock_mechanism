#include "space.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "numericTypes/types.h"
void Space::actor_recordDynamic(const Point3D& point, const ActorIndex& actor, const CollisionVolume& volume)
{
	Actors& actors = m_area.getActors();
	assert(!actors.isStatic(actor));
	auto actorVolumeCopy = m_actorVolume.queryGetOne(point);
	actorVolumeCopy.emplace_back(actor, volume);
	m_actorVolume.insert(point, std::move(actorVolumeCopy));
	auto actorsCopy = m_actors.queryGetOne(point);
	actorsCopy.insert(actor);
	m_actors.insert(point, std::move(actorsCopy));
	auto volumeCopy = m_dynamicVolume.queryGetOne(point);
	volumeCopy += volume;
	m_dynamicVolume.maybeInsert(point, volumeCopy);
}
void Space::actor_recordStatic(const Point3D& point, const ActorIndex& actor, const CollisionVolume& volume)
{
	Actors& actors = m_area.getActors();
	assert(actors.isStatic(actor));
	auto actorVolumeCopy = m_actorVolume.queryGetOne(point);
	actorVolumeCopy.emplace_back(actor, volume);
	m_actorVolume.insert(point, std::move(actorVolumeCopy));
	auto actorsCopy = m_actors.queryGetOne(point);
	actorsCopy.insert(actor);
	m_actors.insert(point, std::move(actorsCopy));
	auto volumeCopy = m_staticVolume.queryGetOne(point);
	volumeCopy += volume;
	m_staticVolume.maybeInsert(point, volumeCopy);
}
void Space::actor_eraseStatic(const Point3D& point, const ActorIndex& actor)
{
	auto actorVolumeCopy = m_actorVolume.queryGetOne(point);
	auto iter = std::ranges::find(actorVolumeCopy, actor, &std::pair<ActorIndex, CollisionVolume>::first);
	CollisionVolume volume = iter->second;
	if(iter != actorVolumeCopy.end())
		(*iter) = actorVolumeCopy.back();
	actorVolumeCopy.pop_back();
	auto actorCopy = m_actors.queryGetOne(point);
	actorCopy.erase(actor);
	m_actors.insert(point, std::move(actorCopy));
	auto volumeCopy = m_staticVolume.queryGetOne(point);
	volumeCopy -= volume;
	m_staticVolume.maybeInsert(point, volumeCopy);
}
void Space::actor_eraseDynamic(const Point3D& point, const ActorIndex& actor)
{
	auto actorVolumeCopy = m_actorVolume.queryGetOne(point);
	auto iter = std::ranges::find(actorVolumeCopy, actor, &std::pair<ActorIndex, CollisionVolume>::first);
	CollisionVolume volume = iter->second;
	if(iter != actorVolumeCopy.end())
		(*iter) = actorVolumeCopy.back();
	actorVolumeCopy.pop_back();
	auto actorCopy = m_actors.queryGetOne(point);
	actorCopy.erase(actor);
	m_actors.insert(point, std::move(actorCopy));
	auto volumeCopy = m_dynamicVolume.queryGetOne(point);
	volumeCopy -= volume;
	m_dynamicVolume.maybeInsert(point, volumeCopy);

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
	m_actorVolume.updateOne(point, [&](std::vector<std::pair<ActorIndex, CollisionVolume>>& data){
		auto iter = std::ranges::find(data, oldIndex, &std::pair<ActorIndex, CollisionVolume>::first);
		iter->first = newIndex;
	});
}
bool Space::actor_contains(const Point3D& point, const ActorIndex& actor) const
{
	return m_actors.queryGetOne(point).contains(actor);
}
bool Space::actor_empty(const Point3D& point) const
{
	return m_actors.queryAny(point);
}
const SmallSet<ActorIndex>& Space::actor_getAll(const Point3D& point) const
{
	return m_actors.queryGetOne(point);
}
