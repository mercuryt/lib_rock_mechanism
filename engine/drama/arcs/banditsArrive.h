#pragma once
#include "../engine.h"
#include "../../eventSchedule.hpp"
#include "../../types.h"
#include "simulation.h"
#include <vector>
class Area;
class Actor;
class BanditsLeaveScheduledEvent;
struct DeserializationMemo;

struct BanditsArriveDramaArc final : public DramaArc
{
	BlockIndex m_entranceBlock;
	ActorReferences m_actors;
	bool m_isActive = false;
	Quantity m_quantity = Quantity::create(0);
	BanditsArriveDramaArc(DramaEngine& engine, Area& area);
	BanditsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& engine);
	void callback();
	[[nodiscard]] Json toJson() const;
	HasScheduledEvent<BanditsLeaveScheduledEvent> m_scheduledEvent;
private:
	ActorReference m_leader;
	void scheduleArrive();
	void scheduleDepart();
	void scheduleContinue();
	//For debug.
	void begin();
};
class BanditsLeaveScheduledEvent final : public ScheduledEvent
{
	BanditsArriveDramaArc& m_dramaticArc;
public:
	BanditsLeaveScheduledEvent(BanditsArriveDramaArc& event, Simulation& simulation, const Step& duration, const Step start = Step::create(1)) : ScheduledEvent(simulation, duration, start), m_dramaticArc(event) { }
	void execute(Simulation&, Area*) { m_dramaticArc.callback(); }
	void clearReferences(Simulation&, Area*) { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};
