#include "conversation.h"
#include "../config/social.h"
#include "../actors/actors.h"
#include "../deserializationMemo.h"
#include "../area/area.h"
#include "../onDestroy.h"
#include "followScript.h"
ConversationObjective::ConversationObjective(Area& area, const ActorIndex& recipient, std::string&& subject, const Priority& priority) :
	Objective(priority),
	m_subject(std::move(subject))
{
	m_recipient.setIndex(recipient, area.getActors().m_referenceData);
	createOnDestroyCallback(area, recipient);
}
ConversationObjective::ConversationObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{
	m_recipient.load(data["recipient"], area.getActors().m_referenceData);
	createOnDestroyCallback(area, m_recipient.getIndex(area.getActors().m_referenceData));
	if(data.contains("duration"))
		m_event.schedule(data["duration"].get<Step>(), *this, area.getActors().getReference(actor), area.m_simulation, data["start"].get<Step>());
}
Json ConversationObjective::toJson() const
{
	Json data = Objective::toJson();
	data["recipient"] = m_recipient;
	return data;
}
void ConversationObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const ActorIndex& recipient = m_recipient.getIndex(actors.m_referenceData);
	actors.drama_castForRole(recipient, *this, actor);
	actors.drama_setDialog( actor, m_subject);
	m_event.schedule(getDuration(), *this, actors.getReference(actor), area.m_simulation);
}
void ConversationObjective::cancel(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
	if(m_event.exists())
	{
		const ActorIndex& recipient = m_recipient.getIndex(actors.m_referenceData);
		actors.objective_complete(recipient, actors.objective_getCurrent<FollowScriptSubObjective>(recipient));
	}
}
void ConversationObjective::delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void ConversationObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void ConversationObjective::callback(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	actors.objective_complete(actor, *this);
	const ActorIndex& recipient = m_recipient.getIndex(actors.m_referenceData);
	actors.objective_complete(recipient, actors.objective_getCurrent<FollowScriptSubObjective>(recipient));
	onComplete(area);
}
void ConversationObjective::createOnDestroyCallback(Area& area, const ActorIndex& actor)
{
	ActorReference actorRef = area.getActors().m_referenceData.getReference(actor);
	m_hasOnDestroySubscriptions.setCallback(std::make_unique<CancelObjectiveOnDestroyCallBack>(actorRef, *this, area));
	// Recipient.
	area.getActors().onDestroy_subscribe(m_recipient.getIndex(area.getActors().m_referenceData), m_hasOnDestroySubscriptions);
}
void ConversationObjective::actorGoesOffScript(Area& area, const ActorIndex& owningActor, const ActorIndex&)
{
	area.getActors().objective_canNotFulfillNeed(owningActor, *this);
}
ConversationScheduledEvent::ConversationScheduledEvent(const Step& duration, ConversationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start) :
	ScheduledEvent(simulation, duration, start),
	m_objective(objective),
	m_actor(actor)
{ }
void ConversationScheduledEvent::execute(Simulation&, Area* area)
{
	m_objective.callback(*area, m_actor.getIndex(area->getActors().m_referenceData));
}
void ConversationScheduledEvent::clearReferences(Simulation&, Area*)
{
	m_objective.m_event.clearPointer();
}