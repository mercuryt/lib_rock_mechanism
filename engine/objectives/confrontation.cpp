#include "confrontation.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../drama/engine.h"
#include "../config/social.h"
#include "../config/psycology.h"
#include "followScript.h"
ConfrontationObjective::ConfrontationObjective(const std::string& reason, const ActorId& target) :
	Objective(Config::Social::socialPriorityHigh),
	m_reason(reason),
	m_target(target)
{ }
ConfrontationObjective::ConfrontationObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{
	data["target"].get_to(m_target);
	data["reason"].get_to(m_reason);
	data["phase"].get_to(m_phase);
	data["targetIsWinner"].get_to(m_targetIsWinner);
	data["targetHasBeenCast"].get_to(m_targetHasBeenCast);
	data["fatal"].get_to(m_fatal);
	data["violent"].get_to(m_violent);
	//createOnDestroyCallback(area, actor);
	if(data.contains("phaseEvent"))
		m_phaseEvent.schedule(data["phaseEvent"]["duration"].get<Step>(), *this, area.getActors().getReference(actor), area.m_simulation, data["phaseEvent"]["start"].get<Step>());
	if(data.contains("actorCoolDownEvent"))
		m_actorCoolDownEvent.schedule(data["actorCoolDownEvent"]["duration"].get<Step>(), *this, area.getActors().getReference(actor), area.m_simulation, data["actorCoolDownEvent"]["start"].get<Step>());
	if(data.contains("targetCoolDownEvent"))
		m_targetCoolDownEvent.schedule(data["targetCoolDownEvent"]["duration"].get<Step>(), *this, area.getActors().getReference(actor), area.m_simulation, data["targetCoolDownEvent"]["start"].get<Step>());
}
Json ConfrontationObjective::toJson() const
{
	Json output = {{"target", m_target}, {"reason", m_reason}, {"phase", m_phase}, {"targetIsWinner", m_targetIsWinner}, {"targetHasBeenCast", m_targetHasBeenCast}, {"fatal", m_fatal}, {"violent", m_violent}};
	if(m_phaseEvent.exists())
	{
		output["phaseEvent"] = Json::object();
		output["phaseEvent"]["duration"] = m_phaseEvent.duration();
		output["phaseEvent"]["start"] = m_phaseEvent.getStartStep();
	};
	if(m_actorCoolDownEvent.exists())
	{
		output["actorCoolDownEvent"] = Json::object();
		output["actorCoolDownEvent"]["duration"] = m_actorCoolDownEvent.duration();
		output["actorCoolDownEvent"]["start"] = m_actorCoolDownEvent.getStartStep();
	};
	if(m_targetCoolDownEvent.exists())
	{
		output["targetCoolDownEvent"] = Json::object();
		output["targetCoolDownEvent"]["duration"] = m_targetCoolDownEvent.duration();
		output["targetCoolDownEvent"]["start"] = m_targetCoolDownEvent.getStartStep();
	};
	return output;
}
void ConfrontationObjective::execute(Area& area, const ActorIndex& actor)
{
	const auto& [actorsAtTargetArea, targetIndex] = area.m_simulation.m_actors.getDataLocation(m_target);
	Actors& actors = area.getActors();
	if(area.m_id == actorsAtTargetArea->getArea().m_id)
	{
		// Actor and target are in the same area.
		if(!actors.isAlive(targetIndex))
		{
			//TODO: If target dies in a different area from actor and actor never goes to that area then objective persists forever.
			actors.drama_setDialog(
				actor,
				"Expresses regret that " + actors.getName(targetIndex) + " cannot be chastised over " + m_reason + " due to death.",
				Config::dialogLineDefaultDurationSteps
			);
			actors.objective_cancel(actor, *this);
			return;
		}
		if(!actors.sleep_isAwake(targetIndex))
		{
			// If sleeping delay till later.
			// TODO: chance to wake them up?
			actors.objective_canNotFulfillNeed(actor, *this);
		}
		if(actors.isAdjacentToActor(actor, targetIndex))
		{
			bool escalate = false;
			if(!m_targetHasBeenCast)
			{
				actors.drama_castForRole(targetIndex, *this, actor);
				m_targetHasBeenCast = true;
			}
			switch(m_phase)
			{
				case ConfrontationPhase::Argument:
				{
					actors.drama_setDialog(
						actor,
						"Argues with " + actors.getName(targetIndex) + " regarding " + m_reason + "."
					);
					escalate = true;
				}
				break;
				case ConfrontationPhase::Insults:
				{
					if(targetYields(area, actor))
					{
						actors.drama_setDialog(
							targetIndex,
							"Backs down from " + actors.getName(actor) + " regarding " + m_reason + "."
						);
						m_targetIsWinner = false;
					}
					else if(actorYields(area, actor))
					{
						actors.drama_setDialog(
							actor,
							"Backs down from " + actors.getName(targetIndex) + " regarding " + m_reason + "."
						);
						m_targetIsWinner = true;
					}
					else
					{
						actors.drama_setDialog(
							actor,
							"Excanges insults with " + actors.getName(targetIndex) + " regarding " + m_reason + "."
						);
						escalate = true;
					}
				}
				break;
				case ConfrontationPhase::Fighting:
					if(targetYields(area, actor))
					{
						actors.drama_setDialog(
							targetIndex,
							"Backs down from " + actors.getName(actor) + " regarding " + m_reason + "."
						);
						m_targetIsWinner = false;
					}
					else if(actorYields(area, actor))
					{
						actors.drama_setDialog(
							actor,
							"Backs down from " + actors.getName(targetIndex) + " regarding " + m_reason + "."
						);
						m_targetIsWinner = true;
					}
					else
					{
						actors.drama_setDialog(
							actor,
							"Is fighting with " + actors.getName(targetIndex) + " regarding " + m_reason + "."
						);
						escalate = true;
						const ActorReference ref = actors.getReference(actor);
						onActorCoolDown(ref, area);
						onTargetCoolDown(ref, area);
					}
				break;
			}
			if(escalate)
				m_phaseEvent.schedule(Config::confrontationObjectiveTempoSteps, *this, actors.getReference(actor), area.m_simulation);
		}
		else
			actors.move_setDestinationAdjacentToActor(actor, targetIndex);
	}
	else
	{
		// Actor and target are not in the same area, suppress the need to keep it active for later.
		actors.objective_canNotFulfillNeed(actor, *this);
	}
}
void ConfrontationObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	if(m_targetHasBeenCast)
	{
		const ActorIndex& target = area.m_simulation.m_actors.getIndexForId(m_target);
		actors.objective_complete(target, actors.objective_getCurrent<FollowScriptSubObjective>(target));
	}
}
void ConfrontationObjective::delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void ConfrontationObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void ConfrontationObjective::onCoolDown(const ActorReference& actorRef, const ActorIndex& thisActor, Area& area)
{
	const auto& [actorsPtr, target] = area.m_simulation.m_actors.getDataLocation(m_target);
	if(actorsPtr != &area.getActors())
	{
		// Target has left area. Reset.
		reset(area, thisActor);
	}
	Actors& actors = *actorsPtr;
	const ActorIndex actor = actorRef.getIndex(actors.m_referenceData);
	// Stop if someone is unconcious or dead.
	if(
		!actors.isAlive(target) || !actors.sleep_isAwake(target) ||
		!actors.isAlive(actor) || !actors.sleep_isAwake(actor)
	)
	{
		finalize(area, actor);
	}
	// Stop if someone backs down.
	else if(targetYields(area, actor))
	{
		actors.drama_setDialog(
			target,
			"Backs down from " + actors.getName(actor) + " regarding " + m_reason + "."
		);
		finalize(area, actor);
	}
	else if(actorYields(area, actor))
	{
		actors.drama_setDialog(
			actor,
			"Backs down from " + actors.getName(target) + " regarding " + m_reason + "."
		);
		finalize(area, actor);
	}
	else
	{
		// Do nonLethal attck and reset cooldown.
		if(actor == target)
		{
			Step coolDown = actors.combat_attackMeleeRangeNonLethal(target, actor);
			m_targetCoolDownEvent.schedule(coolDown, *this, actors.getReference(actor), area.m_simulation);
		}
		else
		{
			Step coolDown = actors.combat_attackMeleeRangeNonLethal(actor, target);
			m_actorCoolDownEvent.schedule(coolDown, *this, actors.getReference(actor), area.m_simulation);
		}
	}
}
void ConfrontationObjective::onActorCoolDown(const ActorReference& ref, Area& area)
{
	Actors& actors = area.getActors();
	ActorIndex actor =  ref.getIndex(actors.m_referenceData);
	onCoolDown(ref, actor, area);
	const auto& [actorsPtr, target] = area.m_simulation.m_actors.getDataLocation(m_target);
	bool finished = false;
	if(!actors.isAlive(target))
	{
		m_targetIsWinner = false;
		finished = true;
		m_fatal = true;
		//area.m_simulation.m_dramaEngine->add(std::make_unique<AccidentalHomicideDramaArc>(area.m_simulation.m_dramaEngine, area, actor, target));
	}
	else if(!actors.sleep_isAwake(target))
	{
		m_targetIsWinner = false;
		finished = true;
	}
	if(finished)
		finalize(area, actor);
}
void ConfrontationObjective::onTargetCoolDown(const ActorReference& actor, Area& area)
{
	const auto& [actorsPtr, target] = area.m_simulation.m_actors.getDataLocation(m_target);
	assert(actorsPtr->getArea().m_id == area.m_id);
	onCoolDown(actor, target, area);
	bool finished = false;
	Actors& actors = *actorsPtr;
	const ActorIndex& actorIndex = actor.getIndex(actors.m_referenceData);
	if(!actors.isAlive(actorIndex))
	{
		m_targetIsWinner = true;
		m_fatal = true;
		finished = true;
		//area.m_simulation.m_dramaEngine->add(std::make_unique<AccidentalHomicideDramaArc>(area.m_simulation.m_dramaEngine, area, target, actor));
	}
	else if(!actors.sleep_isAwake(actorIndex))
	{
		m_targetIsWinner = true;
		finished = true;
	}
	if(finished)
		finalize(area, actorIndex);
}
void ConfrontationObjective::finalize(Area& area, const ActorIndex& actor)
{
	m_phaseEvent.maybeUnschedule();
	m_actorCoolDownEvent.maybeUnschedule();
	m_targetCoolDownEvent.maybeUnschedule();
	const auto& [actorsPtr, target] = area.m_simulation.m_actors.getDataLocation(m_target);
	assert(actorsPtr == &area.getActors());
	Actors& actors = *actorsPtr;
	ActorIndex winner;
	ActorIndex loser;
	if(m_targetIsWinner)
	{
		winner = target;
		loser = actor;
	}
	else
	{
		winner = actor;
		loser = target;
	}
	if(m_fatal)
	{
		actors.psycology_event(winner, PsycologyEventType::AccidentalHomicide, PsycologyAttribute::Pride, -Config::Psycology::prideToLoseWhenAccidentallyKilling, Config::Psycology::durationForAccidentalHomicidePrideLoss);
		//dramaEngine->add(std::make_unique<AccidentalHomicideDramaArc>(dramaEngine, area, actor, target));
	}
	else if(actors.body_isSeriouslyInjured(loser))
	{
		actors.psycology_event(winner, PsycologyEventType::AccidentalSeriousInjury, PsycologyAttribute::Pride, -Config::Psycology::prideToLoseWhenAccidentallyInjuring, Config::Psycology::durationForAccidentalInjuryPrideLoss);
		actors.psycology_event(loser, PsycologyEventType::LoseConfrontation, PsycologyAttribute::Pride, -Config::Social::prideToLoseWhenLosingAConfrontation, Config::Psycology::durationForConfrontationLossPrideLoss);
		//dramaEngine->add(std::make_unique<AccidentalInjuryDramaArc>(dramaEngine, area, winner, loser));
	}
	else
	{
		actors.psycology_event(winner, PsycologyEventType::WinConfrontation, PsycologyAttribute::Pride, Config::Social::prideToAddWhenWinningAConfrontation, Config::Psycology::durationForConfrontationWinPrideGain);
		PsycologyData loserDelta;
		loserDelta.setAllToZero();
		loserDelta.addTo(PsycologyAttribute::Pride, -Config::Social::prideToLoseWhenLosingAConfrontation);
		loserDelta.addTo(PsycologyAttribute::Anger, Config::Psycology::angerToAddWhenLosingAConfrontation);
		actors.psycology_event(loser, PsycologyEventType::LoseConfrontation, loserDelta, Config::Psycology::loseConfrontationDuration);
	}
	actors.objective_complete(target, actors.objective_getCurrent<FollowScriptSubObjective>(target));
	actors.objective_complete(actor, *this);
}
void ConfrontationObjective::actorGoesOffScript(Area& area, const ActorIndex& owningActor, const ActorIndex&)
{
	area.getActors().objective_canNotFulfillNeed(owningActor, *this);
}
bool ConfrontationObjective::targetYields(Area& area, const ActorIndex& actor) const
{
	const auto& [actorsAtTargetArea, targetIndex] = area.m_simulation.m_actors.getDataLocation(m_target);
	assert(actorsAtTargetArea->getArea().m_id == area.m_id);
	// Check if too tired to keep fighting.
	if(actorsAtTargetArea->stamina_empty(actor))
		return true;
	const Psycology& psycology = actorsAtTargetArea->psycology_getConst(targetIndex);
	const PsycologyWeight& pain = actorsAtTargetArea->body_getPain(targetIndex);
	return psycology.getValueFor(PsycologyAttribute::Courage) + psycology.getValueFor(PsycologyAttribute::Anger) > pain;
}
bool ConfrontationObjective::actorYields(Area& area, const ActorIndex& actor) const
{
	Actors& actors = area.getActors();
	const Psycology& psycology = actors.psycology_getConst(actor);
	const PsycologyWeight& pain = actors.body_getPain(actor);
	return psycology.getValueFor(PsycologyAttribute::Courage) + psycology.getValueFor(PsycologyAttribute::Anger) > pain;
}
ConfrontationPhaseScheduledEvent::ConfrontationPhaseScheduledEvent(const Step& duration, ConfrontationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start) :
	ScheduledEvent(simulation, duration, start),
	m_objective(objective),
	m_actor(actor)
{ }
void ConfrontationPhaseScheduledEvent::execute(Simulation&, Area* area)
{
	m_objective.execute(*area, m_actor.getIndex(area->getActors().m_referenceData));
}
void ConfrontationPhaseScheduledEvent::clearReferences(Simulation&, Area*)
{
	m_objective.m_phaseEvent.clearPointer();
}
ConfrontationCoolDownActorScheduledEvent::ConfrontationCoolDownActorScheduledEvent(const Step& duration, ConfrontationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start) :
	ScheduledEvent(simulation, duration, start),
	m_objective(objective),
	m_actor(actor)
{ }
void ConfrontationCoolDownActorScheduledEvent::execute(Simulation&, Area* area)
{
	m_objective.onActorCoolDown(m_actor, *area);
}
void ConfrontationCoolDownActorScheduledEvent::clearReferences(Simulation&, Area*)
{
	m_objective.m_actorCoolDownEvent.clearPointer();
}
ConfrontationCoolDownTargetScheduledEvent::ConfrontationCoolDownTargetScheduledEvent(const Step& duration, ConfrontationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start) :
	ScheduledEvent(simulation, duration, start),
	m_objective(objective),
	m_actor(actor)
{ }
void ConfrontationCoolDownTargetScheduledEvent::execute(Simulation&, Area* area)
{
	m_objective.onTargetCoolDown(m_actor, *area);
}
void ConfrontationCoolDownTargetScheduledEvent::clearReferences(Simulation&, Area*)
{
	m_objective.m_actorCoolDownEvent.clearPointer();
}