#pragma once
#include "eventSchedule.h"
template<class DerivedBlock, class DerivedActor, class DerivedArea>
class MistDisperseEvent : public ScheduledEvent
{
public:
	const FluidType* m_fluidType;
	DerivedBlock& m_block;
	static ScheduledEvent* emplace(EventSchedule& es, uint32_t delay, const FluidType* ft, DerivedBlock& b);
	MistDisperseEvent(uint32_t s, const FluidType* ft, DerivedBlock& b);
	void execute();
	bool continuesToExist() const;
};
