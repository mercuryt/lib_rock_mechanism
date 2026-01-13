/*
	An actor gives a speach. Those who can see at the start are recorded. Those who can still see at the end get a temporary bonus to Courage and Positivity.
*/
#pragma once
#include "../objective.h"
#include "../config/social.h"
#include "../onDestroy.h"
#include "../deserializationMemo.h"

class InspirationalSpeachScheduledEvent;
class InsiprationalSpeachOnAdudienceDestroyCallBack;
class InspirationalSpeachObjective final : public Objective
{
	HasScheduledEvent<InspirationalSpeachScheduledEvent> m_event;
	SmallSet<ActorReference> m_audience;
	HasOnDestroySubscriptions m_audienceOnDestroy;
public:
	InspirationalSpeachObjective(const Priority& priority = Config::Social::socialPriorityHigh);
	InspirationalSpeachObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area& area, const ActorIndex& actor) override;
	void delay(Area& area, const ActorIndex& actor) override;
	void reset(Area& area, const ActorIndex& actor) override;
	void createOnDestroy(Area& area);
	void callback(Area& area, const ActorIndex& actor);
	void removeAudienceMember(const ActorReference& actor);
	[[nodiscard]] constexpr std::string name() const override { return "inspirational speach"; }
	[[nodiscard]] Json toJson() const override;
	friend class InspirationalSpeachScheduledEvent;
	friend class InsiprationalSpeachOnAdudienceDestroyCallBack;
};
class InspirationalSpeachScheduledEvent final : public ScheduledEvent
{
	InspirationalSpeachObjective& m_objective;
	ActorReference m_actor;
public:
	InspirationalSpeachScheduledEvent(const Step& duration, InspirationalSpeachObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start = Step::null());
	void execute(Simulation& simulation, Area* area) override;
	void clearReferences(Simulation& simulation, Area* area) override;
};
class InsiprationalSpeachOnAudienceDestroyCallBack final : public OnDestroyCallBack
{
	InspirationalSpeachObjective* m_objective;
public:
	InsiprationalSpeachOnAudienceDestroyCallBack(InspirationalSpeachObjective& objective) : m_objective(&objective) { }
	InsiprationalSpeachOnAudienceDestroyCallBack(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void callback(const ActorOrItemReference& destroyed);
	[[nodiscard]] Json toJson() const;
};