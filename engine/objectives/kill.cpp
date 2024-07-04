#include "kill.h"
#include "../area.h"
#include "../objective.h"
#include "../visionUtil.h"
#include "../simulation.h"
#include "../simulation/hasActors.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include <memory>
/*
KillInputAction::KillInputAction(std::unordered_set<ActorIndex> actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, ActorIndex killer, ActorIndex target) : InputAction(actors, emplacementType, inputQueue), m_killer(killer), m_target(target) 
{
       	m_onDestroySubscriptions.subscribe(m_target.m_onDestroy); 
}
void KillInputAction::execute()
{
	std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(m_killer, m_target);
	insertObjective(std::move(objective), m_killer);
}
*/
KillObjective::KillObjective(ActorIndex k, ActorIndex t) :
	Objective(k, Config::killPriority), m_killer(k), m_target(t) { }
/*
KillObjective::KillObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), 
	m_killer(deserializationMemo.m_simulation.m_hasActors->getById(data["killer"].get<ActorId>())), 
	m_target(deserializationMemo.m_simulation.m_hasActors->getById(data["target"].get<ActorId>())), 
	m_getIntoRangeAndLineOfSightThreadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine)
{ 
	if(data["threadedTask"])
		m_getIntoRangeAndLineOfSightThreadedTask.create(m_killer, m_target, m_killer.m_canFight.getMaxRange());
}
*/
void KillObjective::execute(Area& area)
{
	Actors& actors = area.getActors();
	if(!actors.isAlive(m_target))
	{
		//TODO: Do we need to cancel the threaded task here?
		actors.objective_complete(m_killer, *this);
		return;
	}
	actors.combat_setTarget(m_killer, m_target);
	// If not in range create GetIntoRangeThreadedTask.
	Blocks& blocks = area.getBlocks();
	if(!actors.move_hasPathRequest(m_actor) &&
			(blocks.taxiDistance(actors.getLocation(m_killer), actors.getLocation(m_target)) > actors.combat_getMaxRange(m_killer) ||
			 // TODO: hasLineOfSightIncludingActors
			 area.m_opacityFacade.hasLineOfSight(actors.getLocation(m_killer), actors.getLocation(m_target)))
	)
		actors.combat_getIntoRangeAndLineOfSightOfActor(m_killer, m_target, actors.combat_getMaxRange(m_killer));
	else
		// If in range and has line of sight and attack not on cooldown then attack.
		if(!actors.combat_isOnCoolDown(m_killer))
			actors.combat_attackMeleeRange(m_killer, m_target);
}
void KillObjective::cancel(Area& area)
{
	area.getActors().move_pathRequestMaybeCancel(m_actor);
}
void KillObjective::reset(Area& area) 
{ 
	cancel(area); 
	area.getActors().canReserve_clearAll(m_killer);
}
