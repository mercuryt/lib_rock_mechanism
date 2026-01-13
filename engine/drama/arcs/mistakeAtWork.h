/*
	Peridically run callback, where we iterate each faction looking for a project to have a mistake occour at.
	May spawn a ChastiseObjective and an AccidentalHomicideDramaArc or an AccidentalInjuryDramaArc.
	TODO: Give them seperate event timers.
*/
#pragma once
#include "../engine.h"
#include "../../eventSchedule.hpp"
#include "../../numericTypes/types.h"
#include "../../reference.h"
class Area;
struct AnimalSpecies;
struct DeserializationMemo;
class MistakeAtWorkScheduledEvent;
enum class MistakeAtWorkType
{
	WastedTime,
	DestroyedInput,
	DamagedUnconsumedItem,
	HurtSomeone,
	Null
};
struct MistakeAtWorkDramaArc final : public DramaArc
{
	HasScheduledEvent<MistakeAtWorkScheduledEvent> m_scheduledEvent;
	MistakeAtWorkDramaArc(DramaEngine& engine, Area& area);
	MistakeAtWorkDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	void callback();
	void schedule();
	[[nodiscard]] std::pair<ActorReference, std::string> doSwitchMaybeReturnVictim(const MistakeAtWorkType& mistakeType, Project& project, const ActorIndex& perpetrator);
	[[nodiscard]] Json toJson() const;
};
class MistakeAtWorkScheduledEvent final : public ScheduledEvent
{
	MistakeAtWorkDramaArc& m_dramaticArc;
public:
	MistakeAtWorkScheduledEvent(const Step& duration, MistakeAtWorkDramaArc& event, Simulation& simulation, const Step start = Step::null()) :
		ScheduledEvent(simulation, duration, start),
		m_dramaticArc(event)
	{ }
	void execute(Simulation&, Area*) { m_dramaticArc.callback(); }
	void clearReferences(Simulation&, Area*) { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};