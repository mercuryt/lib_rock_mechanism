#include "chastise.h"
#include "../actors/actors.h"
#include "../deserializationMemo.h"
#include "../area/area.h"
#include "../items/items.h"
#include "../onDestroy.h"
#include "../config/social.h"
#include "../config/psycology.h"
#include "confrontation.h"
#include "followScript.h"
ChastiseObjective::ChastiseObjective(Area& area, const ActorIndex& recipient, std::string&& subject, const SkillTypeId& skill, int8_t duration, bool angry) :
	Objective(Config::Social::socialPriorityHigh),
	m_subject(std::move(subject)),
	m_skillBeingCritisized(skill),
	m_duration(duration),
	m_angry(angry)
{
	m_recipient.setIndex(recipient, area.getActors().m_referenceData);
	createOnDestroyCallback(area, recipient);
}
ChastiseObjective::ChastiseObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{
	m_recipient.load(data["recipient"], area.getActors().m_referenceData);
	createOnDestroyCallback(area, m_recipient.getIndex(area.getActors().m_referenceData));
	if(data.contains("duration"))
		m_event.schedule(data["duration"].get<Step>(), *this, area.getActors().getReference(actor), area.m_simulation, data["start"].get<Step>());
}
Json ChastiseObjective::toJson() const
{
	return {{"angry", m_angry}, {"recipient", m_recipient}};
}
void ChastiseObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const ActorIndex& recipientIndex = m_recipient.getIndex(actors.m_referenceData);
	// TODO: delay if not awake.
	if(!actors.isAlive(recipientIndex) || !actors.sleep_isAwake(recipientIndex))
	{
		actors.objective_cancel(actor, *this);
		return;
	}
	auto castingCall = [&](const ActorReference& ref) -> float
	{
		const ActorIndex candidate = ref.getIndex(actors.m_referenceData);
		float output = 0;
		Psycology& psycology = actors.psycology_get(candidate);
		const ActorId& candidateId = actors.getId(candidate);
		if(psycology.getFriends().contains(candidateId))
			output += Config::chastiseCastingCallBonusForDefenderIsFriend;
		if(psycology.getFamily().contains(candidateId))
			output += Config::chastiseCastingCallBonusForDefenderIsFamily;
		RelationshipBetweenActors* relationship = psycology.getRelationships().maybeGetForActor(candidateId);
		if(relationship != nullptr)
			output += relationship->positivity.get();
		output += psycology.getValueFor(PsycologyAttribute::Anger).get();
		return output;
	};
	const auto& recipientName = actors.getName(recipientIndex);
	if(actors.distanceToActor(actor, recipientIndex) > Config::maxChastiseDistance)
		actors.move_setDestinationAdjacentToActor(actor, recipientIndex);
	else
	{
		if(!m_event.exists())
		{
			m_event.schedule(Config::stepsPerSecond, *this, actors.getReference(actor), area.m_simulation);
			const auto& actorName = actors.getName(actor);
			actors.drama_setDialog(actor, "chastises " + recipientName + " for " + m_subject + "");
			actors.drama_castForRole(recipientIndex, *this, actor);
		}
		else
		{
			if(m_duration != 0)
			{
				// TODO: change dialog message.
				SmallSet<ActorReference>& canBeSeenBy = actors.vision_getCanBeSeenBy(recipientIndex);
				const ActorReference& interventionCandidate = *std::ranges::max_element(canBeSeenBy.m_data, {}, castingCall);
				if(interventionCandidate.exists() && castingCall(interventionCandidate) >= Config::chastiseCastingCallDefenderMinimumScore)
				{
					// Someone interveins on behalf of recipient.
					m_duration = 0;
					const ActorIndex& defender = interventionCandidate.getIndex(actors.m_referenceData);
					std::string reasonForConfrontation = actors.getName(defender) + " defends " + recipientName + " regarding " + m_subject;
					actors.objective_addNeed(defender, std::make_unique<ConfrontationObjective>(reasonForConfrontation, actors.getId(actor)));
				}
				else
					--m_duration;
				m_event.schedule(Config::stepsPerSecond, *this, actors.getReference(actor), area.m_simulation);

			}
			else
			{
				// Done.
				actors.drama_clearDialog(actor);
				// Recipient get skill points if they are being critisized for poor performance in a skill by someone more skilled then themselves.
				// Actor also gets skill points.
				if(m_skillBeingCritisized.exists() && actors.skill_getLevel(recipientIndex, m_skillBeingCritisized) < actors.skill_getLevel(actor, m_skillBeingCritisized))
				{
					actors.skill_addXp(recipientIndex, m_skillBeingCritisized, Config::skillPointsToAddWhenChastised);
					actors.skill_addXp(actor, m_skillBeingCritisized, Config::skillPointsToAddWhenChastising);
				}
				PsycologyData recipientDelta;
				recipientDelta.setAllToZero();
				// Recipient gets points in shame.
				recipientDelta.addTo(PsycologyAttribute::Pride, -Config::Social::prideToLoseWhenChastised);
				// Recipient may get points in anger if their pride is high or if they are being chastised angrily.
				if(m_angry || recipientDelta.getValueFor(PsycologyAttribute::Pride) > Config::Social::minimumPrideToAlwaysGetAngryWhenChastised)
					recipientDelta.addTo(PsycologyAttribute::Anger, Config::Psycology::angerToAddWhenChastised);
				actors.psycology_event(recipientIndex, PsycologyEventType::Chastised, recipientDelta, Config::Social::psycologyEventChastisedDuration);
				//TODO: Actor psycology.
				actors.objective_complete(actor, *this);
				actors.objective_complete(recipientIndex, actors.objective_getCurrent<FollowScriptSubObjective>(recipientIndex));

			}
		}
	}
}
void ChastiseObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	if(m_event.exists())
	{
		const ActorIndex& recipient = m_recipient.getIndex(actors.m_referenceData);
		actors.objective_complete(recipient, actors.objective_getCurrent<FollowScriptSubObjective>(recipient));
	}
	actors.objective_canNotFulfillNeed(actor, *this);
}
void ChastiseObjective::delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void ChastiseObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void ChastiseObjective::createOnDestroyCallback(Area& area, const ActorIndex& actor)
{
	ActorReference actorRef = area.getActors().m_referenceData.getReference(actor);
	m_hasOnDestroySubscriptions.setCallback(std::make_unique<CancelObjectiveOnDestroyCallBack>(actorRef, *this, area));
	// Recipient.
	area.getActors().onDestroy_subscribe(m_recipient.getIndex(area.getActors().m_referenceData), m_hasOnDestroySubscriptions);
}
ChastiseScheduledEvent::ChastiseScheduledEvent(const Step& duration, ChastiseObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start) :
	ScheduledEvent(simulation, duration, start),
	m_objective(objective),
	m_actor(actor)
{ }
void ChastiseScheduledEvent::execute(Simulation&, Area* area)
{
	m_objective.execute(*area, m_actor.getIndex(area->getActors().m_referenceData));
}
void ChastiseScheduledEvent::clearReferences(Simulation&, Area*)
{
	m_objective.m_event.clearPointer();
}