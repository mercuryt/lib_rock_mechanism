#include "squad.h"
#include "definitions/shape.h"
#include "area/area.h"
#include "actors/actors.h"
const std::string& SquadFormation::getName() const { return m_name; }
const ShapeId& SquadFormation::getShapeId() const { return m_shapeId; }
const Offset3D SquadFormation::getOffsetFor(const ActorId& reference) const { return m_locations[reference]; }
void SquadFormation::setName(const std::string& name) { m_name = name; }
void SquadFormation::setOffset(const ActorId& actor, const Offset3D& offset) { m_locations.getOrCreate(actor) = offset; }
void SquadFormation::remove(const ActorId& actor) { m_locations.erase(actor); }
void Squad::setCommander(const ActorId& commander) { m_commander = commander; }
void Squad::addActor(const ActorId& actor) { m_actors.insert(actor); }
// Remove from all formations.
void Squad::removeActor(const ActorId& actor)
{
	m_actors.erase(actor);
	for(SquadFormation& formation : m_formations)
		formation.remove(actor);
}
void Squad::setOffsetOfActorInCurrentFormation(const ActorId& actor, const Offset3D& offset)
{
	assert(m_currentFormationIndex.exists());
	m_formations[m_currentFormationIndex].setOffset(actor, offset);
}
void Squad::setName(const std::string& name) { m_name = name; }
void Squad::createFormationWithCurrentPositions(const std::string& name, Area& area)
{
	SquadFormation formation;
	formation.setName(name);
	Actors& actors = area.getActors();
	const SimulationHasActors& simulationHasActors = area.m_simulation.m_actors;
	const ActorIndex& commander = simulationHasActors.getIndexForId(m_commander);
	Point3D commanderLocation = actors.getLocation(commander);
	for(const ActorId& soldierId : m_actors)
	{
		const ActorIndex& soldier = simulationHasActors.getIndexForId(soldierId);
		Point3D soldierLocation = actors.getLocation(soldier);
		Offset3D offset = commanderLocation.offsetTo(soldierLocation);
		formation.setOffset(soldierId, offset);
	}
	m_formations.add(std::move(formation));
}
void Squad::updateActorIndex(const ActorIndex& oldIndex, const ActorIndex& newIndex)
{
	m_currentFormationLocationsAsIndices.updateKey(oldIndex, newIndex);
}
void Squad::setCurrentArea(const Area& area)
{
	if(m_currentFormationIndex.empty())
		return;
	m_currentFormationLocationsAsIndices.clear();
	const SimulationHasActors& hasActors = area.m_simulation.m_actors;
	for(const auto& [actorId, offset3D] : m_formations[m_currentFormationIndex].m_locations)
	{
		m_currentFormationLocationsAsIndices.insert(hasActors.getIndexForId(actorId), offset3D);
	}
}
const std::string& Squad::getName() const { return m_name; }
ActorId& Squad::getCommander() { return m_commander; }
const ActorId& Squad::getCommander() const { return m_commander; }
const SmallSet<ActorId>& Squad::getAll() const { return m_actors; }
SquadFormation* Squad::getFormation()
{
	if(m_currentFormationIndex.empty())
		return nullptr;
	return &m_formations[m_currentFormationIndex];
}
const SquadFormation* Squad::getFormation() const { return const_cast<Squad*>(this)->getFormation(); }
SquadFormation& Squad::getFormationByName(std::string& name)
{
	for(SquadFormation& formation : m_formations)
		if(formation.m_name == name)
			return formation;
	assert(false);
	std::unreachable();
}
const StrongVector<SquadFormation, SquadFormationIndex>& Squad::getFormations() const { return m_formations; }
Squad Squad::create(const ActorId& commanderId, const std::string& name)
{
	Squad output;
	output.setCommander(commanderId);
	output.addActor(commanderId);
	output.setName(name);
	return output;
}