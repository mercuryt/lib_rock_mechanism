#include "kill.h"
#include "../area/area.h"
#include "../objective.h"
#include "../simulation/simulation.h"
#include "../simulation/hasActors.h"
#include "actors/actors.h"
#include "space/space.h"
#include "reference.h"
#include <memory>
KillObjective::KillObjective(ActorReference t) : Objective(Config::killPriority), m_target(t) { }
KillObjective::KillObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Objective(data, deserializationMemo),
	m_target(data["target"], area.getActors().m_referenceData) { }
void KillObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	ActorIndex target = m_target.getIndex(actors.m_referenceData);
	if(!actors.isAlive(target))
	{
		//TODO: Do we need to cancel the threaded task here?
		actors.objective_complete(actor, *this);
		return;
	}
	actors.combat_setTarget(actor, target);
	// If not in range create GetIntoRangeThreadedTask.
	if(
		!actors.move_hasPathRequest(actor) &&
		(
			actors.getLocation(actor).distanceToFractional(actors.getLocation(target)) > actors.combat_getMaxRange(actor) ||
			// TODO: hasLineOfSightIncludingActors
			!area.m_opacityFacade.hasLineOfSight(actors.getLocation(actor), actors.getLocation(target))
		)
	)
		actors.combat_getIntoRangeAndLineOfSightOfActor(actor, target, actors.combat_getMaxRange(actor));
	else
		// If in range and has line of sight and attack not on cooldown then attack.
		if(!actors.combat_isOnCoolDown(actor))
			actors.combat_attackMeleeRange(actor, target);
}
void KillObjective::cancel(Area& area, const ActorIndex& actor)
{
	area.getActors().move_pathRequestMaybeCancel(actor);
}
void KillObjective::reset(Area& area, const ActorIndex& actor)
{
	cancel(area, actor);
	area.getActors().canReserve_clearAll(actor);
}
