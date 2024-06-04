#pragma once
#include "eventSchedule.hpp"
#include "types.h"
#include "config.h"
#include <array>
class Area;
class RainEvent;
struct FluidType;
struct DeserializationMemo;
class AreaHasRain final
{
	std::array<Percent, 4> m_humidityBySeason;
	HasScheduledEvent<RainEvent> m_event;
	Area& m_area;
	const FluidType* m_currentlyRainingFluidType = nullptr;
	const FluidType& m_defaultRainFluidType;
	Percent m_intensityPercent = 0;
public:
	AreaHasRain(Area& a, Simulation& s);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void start(const FluidType& fluidType, Percent intensityPercent, Step stepsDuration);
	void start(Percent intensityPercent, Step stepsDuration) { start(m_defaultRainFluidType, intensityPercent, stepsDuration); }
	void schedule(Step restartAt) { m_event.schedule(restartAt, *this); }
	void stop();
	void writeStep();
	void scheduleRestart();
	void disable();
	[[nodiscard]] bool isRaining() const { return m_intensityPercent != 0; }
	[[nodiscard]] const FluidType& getFluidType() const { assert(m_currentlyRainingFluidType); return *m_currentlyRainingFluidType; }
	[[nodiscard]] Percent getIntensityPercent() const { return m_intensityPercent; }
	[[nodiscard]] Percent humidityForSeason();
	friend class RainEvent;
};
class RainEvent final : public ScheduledEvent
{
	AreaHasRain& m_areaHasRain;
public:
	RainEvent(Step delay, AreaHasRain& ahr, const Step start = 0);
	void execute();
	void clearReferences() { m_areaHasRain.m_event.clearPointer(); }
};
