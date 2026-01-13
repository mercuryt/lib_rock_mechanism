#include "inspirationalSpeach.h"
#include "../numericTypes/types.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../config/social.h"
InspirationalSpeachObjective::InspirationalSpeachObjective(const Priority& priority) : Objective(priority) { }
InspirationalSpeachObjective::InspirationalSpeachObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_audience(data["audience"])
{
	if(data.contains("duration"))
		m_event.schedule(data["duration"].get<Step>(), *this, area.getActors().getReference(actor), area.m_simulation, data["start"].get<Step>());
	createOnDestroy(area);
}
void InspirationalSpeachObjective::execute(Area& area, const ActorIndex& actor)
{
	m_event.schedule(Config::Social::inspirationalSpeachDuration, *this, area.getActors().getReference(actor), area.m_simulation);
	// Record audience at start.
	Actors& actors = area.getActors();
	const FactionId& actorFaction = actors.getFaction(actor);
	for(const ActorReference& watcher : actors.vision_getCanBeSeenBy(actor))
		if(actors.getFaction(watcher.getIndex(actors.m_referenceData)) == actorFaction)
			m_audience.insert(watcher);
}
void InspirationalSpeachObjective::cancel(Area&, const ActorIndex&) { m_event.maybeUnschedule(); m_audience.clear(); }
void InspirationalSpeachObjective::delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void InspirationalSpeachObjective::reset(Area& area, const ActorIndex& actor) { cancel(area, actor); }
void InspirationalSpeachObjective::createOnDestroy(Area& area)
{
	Actors& actors = area.getActors();
	for(const ActorReference& ref : m_audience)
		actors.onDestroy_subscribe(ref.getIndex(actors.m_referenceData), m_audienceOnDestroy);
	m_audienceOnDestroy.setCallback(std::make_unique<InsiprationalSpeachOnAudienceDestroyCallBack>(*this));
}
void InspirationalSpeachObjective::callback(Area& area, const ActorIndex& actor)
{
	// Apply bonus to audience at start who are still here at the end.
	Actors& actors = area.getActors();
	ActorReference speakerRef = actors.getReference(actor);
	for(const ActorReference& ref : m_audience)
	{
		const ActorIndex& watcher = ref.getIndex(actors.m_referenceData);
		if(!actors.isAlive(watcher) || !actors.sleep_isAwake(watcher))
			continue;
		if(!actors.vision_getCanSee(watcher).contains(speakerRef))
			continue;
		PsycologyData delta;
		delta.addTo(PsycologyAttribute::Courage, Config::Social::insprationalSpeachCourageToAdd);
		delta.addTo(PsycologyAttribute::Positivity, Config::Social::insprationalSpeachPositivityToAdd);
		actors.psycology_event(watcher, PsycologyEventType::InspirationalSpeach, delta, Config::Social::inspirationalSpeachEffectDuration);
	}
}
void InspirationalSpeachObjective::removeAudienceMember(const ActorReference& actor)
{
	m_audience.erase(actor);
}
Json InspirationalSpeachObjective::toJson() const
{
	Json output = Objective::toJson();
	output["audience"] = m_audience;
	if(m_event.exists())
	{
		output["duration"] = m_event.duration();
		output["start"] = m_event.getStartStep();
	}
	return output;
}
InspirationalSpeachScheduledEvent::InspirationalSpeachScheduledEvent(const Step& duration, InspirationalSpeachObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start) :
	ScheduledEvent(simulation, duration, start),
	m_objective(objective),
	m_actor(actor)
{ }
void InspirationalSpeachScheduledEvent::execute(Simulation&, Area* area)
{
	m_objective.callback(*area, m_actor.getIndex(area->getActors().m_referenceData));
}
void InspirationalSpeachScheduledEvent::clearReferences(Simulation&, Area*)
{
	m_objective.m_event.clearPointer();
}
InsiprationalSpeachOnAudienceDestroyCallBack::InsiprationalSpeachOnAudienceDestroyCallBack(const Json& data, DeserializationMemo& deserializationMemo, Area&) :
	m_objective((InspirationalSpeachObjective*)&*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>()))
{ }
[[nodiscard]] Json InsiprationalSpeachOnAudienceDestroyCallBack::toJson() const
{
	return {{"objective", m_objective}};
}
void InsiprationalSpeachOnAudienceDestroyCallBack::callback(const ActorOrItemReference& destroyed)
{
	m_objective->removeAudienceMember(destroyed.toActorReference());
}
