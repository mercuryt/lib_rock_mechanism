#pragma once
#include "eventSchedule.h"
#include "temperatureSource.h"
#include <memory>

constexpr int32_t heatFractionForSmoulder = 10;
constexpr int32_t heatFractionForBurn = 3;
constexpr int32_t rampDownPhaseDurationFraction = 2;

enum class FireStage{Smouldering, Burining, Flaming};

template<class Block, class MaterialType> class Fire;

/*
 * Progress through the stages of fire with scheduled events. Fire stores a pointer to the current event so it can be canceled if it is deleted ( extinguished ).
 */
template<class Fire>
class FireEvent : public ScheduledEvent
{
public:
	Fire& m_fire;

	static ScheduledEvent* emplace(EventSchedule& es, uint32_t delaySteps, Fire& f)
	{
		std::unique_ptr<ScheduledEvent> event = std::make_unique<FireEvent<Fire>>(s_step + delaySteps, f);
		ScheduledEvent* output = event.get();
		es.schedule(std::move(event));
		return output;
	}
	FireEvent(uint32_t s, Fire& f) : ScheduledEvent(s), m_fire(f) {}
	void execute()
	{
		if(!m_fire.m_hasPeaked &&m_fire.m_stage == FireStage::Smouldering)
		{
			m_fire.m_stage = FireStage::Burining;
			m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature / heatFractionForBurn);
			m_fire.m_event = emplace(m_fire.m_location->m_area->m_eventSchedule, m_fire.m_materialType.burnStageDuration, m_fire);
		}
		else if(!m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burining)
		{
			m_fire.m_stage = FireStage::Flaming;
			m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature);
			m_fire.m_event = emplace(m_fire.m_location->m_area->m_eventSchedule, m_fire.m_materialType.flameStageDuration, m_fire);
			//TODO: schedule event to turn construction / solid into wreckage.
		}
		else if(m_fire.m_stage == FireStage::Flaming)
		{
			m_fire.m_hasPeaked = true;
			m_fire.m_stage = FireStage::Burining;
			m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature / heatFractionForBurn);
			uint32_t delay = m_fire.m_materialType.burnStageDuration / rampDownPhaseDurationFraction;
			m_fire.m_event = emplace(m_fire.m_location->m_area->m_eventSchedule, delay, m_fire);
		}
		else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burining)
		{
			m_fire.m_stage = FireStage::Smouldering;
			m_fire.m_temperatureSource.setTemperature(m_fire.m_materialType.flameTemperature / heatFractionForSmoulder);
			uint32_t delay = m_fire.m_materialType.burnStageDuration / rampDownPhaseDurationFraction;
			m_fire.m_event = emplace(m_fire.m_location->m_area->m_eventSchedule, delay, m_fire);
		}
		else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Smouldering)
		{
			// Clear the event pointer so ~Fire doesn't try to cancel the event which is currently executing.
			m_fire.m_event = nullptr;
			// Destroy m_fire by relasing the unique pointer in m_location.
			// Implicitly removes the influence of m_fire.m_temperatureSource.
			m_fire.m_location->m_fire.release();
		}
	}
	~FireEvent()
	{
		m_fire.m_event = nullptr;
	}
};
template<class Block, class MaterialType>
class Fire
{
public:
	Block* m_location;
	const MaterialType& m_materialType;
	ScheduledEvent* m_event;
	FireStage m_stage;
	bool m_hasPeaked;
	TemperatureSource<Block> m_temperatureSource;

	Fire(Block& l, const MaterialType& mt) : m_location(&l), m_materialType(mt), m_stage(FireStage::Smouldering), m_hasPeaked(false), m_temperatureSource(m_materialType.flameTemperature / heatFractionForSmoulder, l)
	{
		m_event = FireEvent<Fire>::emplace(m_location->m_area->m_eventSchedule, m_materialType.burnStageDuration, *this);
	}
	~Fire()
	{
		if(m_event != nullptr)
			m_event->cancel();
	}
};
