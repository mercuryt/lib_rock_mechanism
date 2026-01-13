/*
	Peridically run callback, where we iterate each faction looking for a project to have a success occour at.
	May spawn a PraiseObjective.
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
class SuccessAtWorkScheduledEvent;
enum class SuccessAtWorkType
{
	SavedTime,
	SavedInputItems,
	HighQuality, // Most project types won't be able to accomidate this result and will reduce to SavedInputItems instead.
	Null
};
struct SuccessAtWorkDramaArc final : public DramaArc
{
	HasScheduledEvent<SuccessAtWorkScheduledEvent> m_scheduledEvent;
	SuccessAtWorkDramaArc(DramaEngine& engine, Area& area);
	SuccessAtWorkDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	void callback();
	void schedule();
	std::string doSwitch(const SuccessAtWorkType& successType, Project& project);
	[[nodiscard]] Json toJson() const;
};
class SuccessAtWorkScheduledEvent final : public ScheduledEvent
{
	SuccessAtWorkDramaArc& m_dramaticArc;
public:
	SuccessAtWorkScheduledEvent(const Step& duration, SuccessAtWorkDramaArc& event, Simulation& simulation, const Step start = Step::null()) :
		ScheduledEvent(simulation, duration, start),
		m_dramaticArc(event)
	{ }
	void execute(Simulation&, Area*) { m_dramaticArc.callback(); }
	void clearReferences(Simulation&, Area*) { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};