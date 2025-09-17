#include "space.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../numericTypes/types.h"
void Space::actor_recordDynamic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor)
{
	assert(!toOccupy.empty());
	Actors& actors = m_area.getActors();
	assert(!actors.isStatic(actor));
	for(const auto& [cuboid, volume] : toOccupy)
		m_actors.insert(cuboid, actor);
	for(const auto& [cuboid, volume] : toOccupy)
		m_dynamicVolume.updateAddAll(cuboid, volume);
}
void Space::actor_recordStatic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor)
{
	assert(!toOccupy.empty());
	Actors& actors = m_area.getActors();
	assert(actors.isStatic(actor));
	for(const auto& [cuboid, volume] : toOccupy)
		m_actors.insert(cuboid, actor);
	for(const auto& [cuboid, volume] : toOccupy)
		m_staticVolume.updateAddAll(cuboid, volume);
}
void Space::actor_eraseDynamic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor)
{
	assert(!toOccupy.empty());
	Actors& actors = m_area.getActors();
	assert(!actors.isStatic(actor));
	for(const auto& [cuboid, volume] : toOccupy)
		m_actors.removeAll(cuboid, actor);
	for(const auto& [cuboid, volume] : toOccupy)
		m_dynamicVolume.updateSubtractAll(cuboid, volume);
}
void Space::actor_eraseStatic(const MapWithCuboidKeys<CollisionVolume>& toOccupy, const ActorIndex& actor)
{
	assert(!toOccupy.empty());
	Actors& actors = m_area.getActors();
	assert(actors.isStatic(actor));
	for(const auto& [cuboid, volume] : toOccupy)
		m_actors.removeAll(cuboid, actor);
	for(const auto& [cuboid, volume] : toOccupy)
		m_staticVolume.updateSubtractAll(cuboid, volume);
}
void Space::actor_setTemperature(const Point3D& point, [[maybe_unused]] const Temperature& temperature)
{
	assert(temperature_get(point) == temperature);
	Actors& actors = m_area.getActors();
	for(const ActorIndex& actor : m_actors.queryGetAll(point))
		actors.temperature_onChange(actor);
}
void Space::actor_updateIndex(const Cuboid& cuboid, const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	m_actors.update(cuboid, oldIndex, newIndex);
}