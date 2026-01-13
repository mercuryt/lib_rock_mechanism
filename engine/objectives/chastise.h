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
	int8_t m_duration;
	bool m_angry;
public:
	ChastiseObjective(Area& area, const ActorIndex& receipent, std::string&& subject, const SkillTypeId& skill, int8_t duration, bool m_angry);
	ChastiseObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area& area, const ActorIndex& actor) override;
	void delay(Area& area, const ActorIndex& actor) override;
	void reset(Area& area, const ActorIndex& actor) override;
	void createOnDestroyCallback(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "chastise"; }
	[[nodiscard]] Json toJson() const;
	friend class ChastiseScheduledEvent;
};
class ChastiseScheduledEvent final : public ScheduledEvent
{
	ChastiseObjective& m_objective;
	ActorReference m_actor;
public:
	ChastiseScheduledEvent(const Step& duration, ChastiseObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
//TODO: add to deserialization memo.