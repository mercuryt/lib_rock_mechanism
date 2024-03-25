#pragma once
#include "../engine.h"
#include "../../eventSchedule.hpp"
#include "../../types.h"
#include <vector>
class Area;
class Actor;
class BanditsLeaveScheduledEvent;
struct DeserializationMemo;
class Block;

struct BanditsArriveDramaArc final : public DramaArc
{
	Block* m_entranceBlock = nullptr;;
	std::vector<Actor*> m_actors;
	bool m_isActive = false;
	uint32_t m_quantity = 0;
	BanditsArriveDramaArc(DramaEngine& engine, Area& area);
	BanditsArriveDramaArc(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void callback();
	HasScheduledEvent<BanditsLeaveScheduledEvent> m_scheduledEvent;
private:
	Actor* m_leader = nullptr;
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
	void execute() { m_dramaticArc.callback(); }
	void clearReferences() { m_dramaticArc.m_scheduledEvent.clearPointer(); }
};
