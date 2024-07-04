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
	BlockIndex m_entranceBlock = BLOCK_INDEX_MAX;
	std::vector<ActorIndex> m_actors;
	bool m_isActive = false;
	uint32_t m_quantity = 0;
	BanditsArriveDramaArc(DramaEngine& engine, Area& area);
	BanditsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& engine);
	[[nodiscard]] Json toJson() const;
	void callback();
	HasScheduledEvent<BanditsLeaveScheduledEvent> m_scheduledEvent;
private:
	ActorIndex m_leader = ACTOR_INDEX_MAX;
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
	BanditsLeaveScheduledEvent(BanditsArriveDramaArc& event, Simulation& simulation, Step duration, Step start = 0) : ScheduledEvent(simulation, duration, start), m_dramaticArc(event) { }
	void execute(Simulation&, Area*) { m_dramaticArc.callback(); }
	void clearReferences(Simulation&, Area*) { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};
