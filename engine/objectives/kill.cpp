#include "kill.h"
#include "../area.h"
#include "../objective.h"
#include "../simulation.h"
#include "../simulation/hasActors.h"
#include "actors/actors.h"
#include "blocks/blocks.h"
#include "reference.h"
#include <memory>
/*
KillInputAction::KillInputAction(ActorIndices actors, NewObjectiveEmplacementType emplacementType, InputQueue& inputQueue, ActorIndex killer, ActorIndex target) : InputAction(actors, emplacementType, inputQueue), m_killer(killer), m_target(target) 
{
       	m_onDestroySubscriptions.subscribe(m_target.m_onDestroy); 
}
void KillInputAction::execute()
{
	std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(m_killer, m_target);
	insertObjective(std::move(objective), m_killer);
}
*/
KillObjective::KillObjective(ActorReference t) : Objective(Config::killPriority), m_target(t) { }
KillObjective::KillObjective(const Json& data, Area& area) : Objective(data)
{ 
	m_target.load(data["target"], area);
}
void KillObjective::execute(Area& area, ActorIndex actor)
{
	Actors& actors = area.getActors();
	ActorIndex target = m_target.getIndex();
	if(!actors.isAlive(target))
	{
		//TODO: Do we need to cancel the threaded task here?
		actors.objective_complete(actor, *this);
		return;
	}
	actors.combat_setTarget(actor, target);
	// If not in range create GetIntoRangeThreadedTask.
	Blocks& blocks = area.getBlocks();
	if(!actors.move_hasPathRequest(actor) &&
			(blocks.distanceFractional(actors.getLocation(actor), actors.getLocation(target)) > actors.combat_getMaxRange(actor) ||
			 // TODO: hasLineOfSightIncludingActors
			 area.m_opacityFacade.hasLineOfSight(actors.getLocation(actor), actors.getLocation(target)))
	)
		actors.combat_getIntoRangeAndLineOfSightOfActor(actor, target, actors.combat_getMaxRange(actor));
	else
		// If in range and has line of sight and attack not on cooldown then attack.
		if(!actors.combat_isOnCoolDown(actor))
			actors.combat_attackMeleeRange(actor, target);
}
void KillObjective::cancel(Area& area, ActorIndex actor)
{
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void KillObjective::reset(Area& area, ActorIndex actor) 
{ 
	cancel(area, actor); 
	area.getActors().canReserve_clearAll(actor);
}
