#pragma once
#include "../objective.h"
#include "../onDestroy.h"
#include "../eventSchedule.hpp"
#include "../numericTypes/idTypes.h"
#include "../config/social.h"

struct ConversationScheduledEvent;
// ConversationObjective is not final.
class ConversationObjective : public Objective
{
	HasOnDestroySubscriptions m_hasOnDestroySubscriptions;
	HasScheduledEvent<ConversationScheduledEvent> m_event;
protected:
	std::string m_subject;
	ActorReference m_recipient;
public:
	ConversationObjective(Area& area, const ActorIndex& receipient, std::string&& subject = "chat", const Priority& priority = Config::Social::socialPriorityLow);
	ConversationObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area& area, const ActorIndex& actor) override;
	void delay(Area& area, const ActorIndex& actor) override;
	void reset(Area& area, const ActorIndex& actor) override;
	void callback(Area& area, const ActorIndex& actor);
	void createOnDestroyCallback(Area& area, const ActorIndex& actor);
	void actorGoesOffScript(Area& area, const ActorIndex& owningActor, const ActorIndex& offScriptActor) override;
	[[nodiscard]] constexpr std::string name() const override { return "conversation"; }
	[[nodiscard]] Json toJson() const override;
	[[nodiscard]] virtual constexpr Step getDuration() const { return Config::Social::conversationDurationSteps; }
	// TODO: provide a default onComplete.
	virtual void onComplete(Area&) { }
	virtual ~ConversationObjective() = default;
	friend class ConversationScheduledEvent;
};
class ConversationScheduledEvent final : public ScheduledEvent
{
	ConversationObjective& m_objective;
	ActorReference m_actor;
public:
	ConversationScheduledEvent(const Step& duration, ConversationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start = Step::null());
	void execute(Simulation& simulation, Area* area) override;
	void clearReferences(Simulation& simulation, Area* area) override;
};
//TODO: add to deserialization memo.