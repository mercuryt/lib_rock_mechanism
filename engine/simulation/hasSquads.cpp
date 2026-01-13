#include "hasSquads.h"
#include "simulation.h"
#include "../actors/actors.h"
#include "../area/area.h"
void SimulationHasSquads::destroy(const FactionId& faction, const SquadIndex& squadIndex, Simulation& simulation)
{
	auto& squads = m_squads[faction];
	if(squads.size() != squadIndex.get() - 1)
	{
		// This is not the last squad. Last squad will be copied over and then it's soldier's squad indices will be updated.
		squads[squadIndex] = squads.back();
		Squad& squad = squads[squadIndex];
		Area& area = simulation.m_actors.getAreaForId(squad.getCommander());
		Actors& actors = area.getActors();
		for(const ActorId& actor : squad.getAll())
		{
			const ActorIndex& index = simulation.m_actors.getIndexForId(actor);
			actors.soldier_setSquad(index, squadIndex);
		}
	}
	squads.popBack();
}
SquadIndex SimulationHasSquads::create(const ActorId& commanderId, const FactionId& faction, const std::string& name)
{
	m_squads[faction].emplaceBack(Squad::create(commanderId, name));
	SquadIndex index{m_squads[faction].size()};
	return index;
}
StrongVector<Squad, SquadIndex>& SimulationHasSquads::squadsForFaction(const FactionId& faction) { return m_squads[faction]; }
const StrongVector<Squad, SquadIndex>& SimulationHasSquads::squadsForFaction(const FactionId& faction) const { return m_squads[faction]; }