#pragma once
#include "../objective.h"
#include "../onDestroy.h"
#include "../eventSchedule.hpp"
#include "../numericTypes/idTypes.h"

struct ChastiseScheduledEvent;
class ChastiseObjective final : public Objective
{
	HasOnDestroySubscriptions m_hasOnDestroySubscriptions;
	HasScheduledEvent<ChastiseScheduledEvent> m_event;
	std::string m_subject;
	ActorReference m_recipient;
	SkillTypeId m_skillBeingCritisized;
	int m_duration;
	bool m_angry;
public:
	ChastiseObjective(Area& area, ActorIndex receipent, std::string&& subject, const SkillTypeId skill, int duration, bool m_angry);
	ChastiseObjective(const Json& data, Area& area, ActorIndex actor, DeserializationMemo& deserializationMemo);
	void execute(Area& area, ActorIndex actor) override;
	void cancel(Area& area, ActorIndex actor) override;
	void delay(Area& area, ActorIndex actor) override;
	void reset(Area& area, ActorIndex actor) override;
	void createOnDestroyCallback(Area& area, ActorIndex actor);
	[[nodiscard]] std::string name() const { return "chastise"; }
	[[nodiscard]] Json toJson() const;
	friend class ChastiseScheduledEvent;
};
class ChastiseScheduledEvent final : public ScheduledEvent
{
	ChastiseObjective& m_objective;
	ActorReference m_actor;
public:
	ChastiseScheduledEvent(const Step duration, ChastiseObjective& objective, const ActorReference actor, Simulation& simulation, const Step start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
//TODO: add to deserialization memo.