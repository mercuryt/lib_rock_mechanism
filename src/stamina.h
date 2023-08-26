#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "config.h"

class RestEvent;
class Actor;

class RestObjective final : public Objective
{
	Actor& m_actor;
	HasScheduledEvent<RestEvent> m_restEvent;
public:
	RestObjective(Actor& a);
	void execute() { m_restEvent.schedule(*this); }
	void cancel() { }
	std::string name() { return "rest"; }
	friend class RestEvent;
};
class RestEvent final : public ScheduledEventWithPercent
{
	RestObjective& m_objective;
public:
	RestEvent(RestObjective& ro);
	void execute();
	void clearReferences() { m_objective.m_restEvent.clearPointer(); }
};

class ActorHasStamina final
{
	Actor& m_actor;
	uint32_t m_stamina;

public:
	ActorHasStamina(Actor& a) : m_actor(a), m_stamina(getMax()) { }
	void recover();
	void spend(uint32_t stamina);
	void setFull();
	bool hasAtLeast(uint32_t stamina) const;
	bool isFull() const;
	uint32_t getMax() const;
};
