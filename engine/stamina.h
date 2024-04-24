#pragma once

#include "objective.h"
#include "eventSchedule.h"
#include "config.h"

class RestEvent;
class Actor;
struct DeserializationMemo;

class RestObjective final : public Objective
{
	HasScheduledEvent<RestEvent> m_restEvent;
public:
	RestObjective(Actor& a);
	RestObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute() { m_restEvent.schedule(*this); }
	void cancel() { m_restEvent.maybeUnschedule(); }
	void delay() { cancel(); }
	void reset();
	std::string name() const { return "rest"; }
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Rest; }
	friend class RestEvent;
};
class RestEvent final : public ScheduledEvent
{
	RestObjective& m_objective;
public:
	RestEvent(RestObjective& ro, const Step start = 0);
	void execute();
	void clearReferences() { m_objective.m_restEvent.clearPointer(); }
};

class ActorHasStamina final
{
	Actor& m_actor;
	uint32_t m_stamina;

public:
	ActorHasStamina(Actor& a) : m_actor(a), m_stamina(getMax()) { }
	ActorHasStamina(Actor& a, uint32_t stamina) : m_actor(a), m_stamina(stamina) { }
	void recover();
	void spend(uint32_t stamina);
	void setFull();
	bool hasAtLeast(uint32_t stamina) const;
	bool isFull() const;
	uint32_t getMax() const;
	uint32_t get() const { return m_stamina; }
};
