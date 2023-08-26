#pragma once
#include "fluidType.h"
#include "eventSchedule.hpp"
class Area;
class StopRainEvent;
class AreaHasRain final
{
	Area& m_area;
	const FluidType* m_currentlyRainingFluidType;
	uint32_t m_intensityPercent;
	HasScheduledEvent<StopRainEvent> m_stopEvent;
public:
	AreaHasRain(Area& a);
	void start(const FluidType& fluidType, uint32_t intensityPercent, uint32_t stepsDuration);
	void stop();
	void writeStep();
	friend class StopRainEvent;
};
class StopRainEvent final : public ScheduledEventWithPercent
{
	AreaHasRain& m_areaHasRain;
public:
	StopRainEvent(uint32_t delay, AreaHasRain& ahr);
	void execute() { m_areaHasRain.stop(); }
	void clearReferences() { m_areaHasRain.m_stopEvent.clearPointer(); }
};
