#include "fire.h"
#include "block.h"
#include "area.h"
void FireEvent::execute()
{
	if(!m_fire.m_hasPeaked &&m_fire.m_stage == FireStage::Smouldering)
	{
		m_fire.m_stage = FireStage::Burining;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature / Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(temperature);
		m_fire.m_event.schedule(m_fire.m_materialType.burnData->burnStageDuration, m_fire);
	}
	else if(!m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burining)
	{
		m_fire.m_stage = FireStage::Flaming;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature;
		m_fire.m_temperatureSource.setTemperature(temperature);
		m_fire.m_event.schedule(m_fire.m_materialType.burnData->flameStageDuration, m_fire);
		//TODO: schedule event to turn construction / solid into wreckage.
	}
	else if(m_fire.m_stage == FireStage::Flaming)
	{
		m_fire.m_hasPeaked = true;
		m_fire.m_stage = FireStage::Burining;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature / Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(temperature);
		uint32_t delay = m_fire.m_materialType.burnData->burnStageDuration / Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(delay, m_fire);
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burining)
	{
		m_fire.m_stage = FireStage::Smouldering;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature / Config::heatFractionForSmoulder;
		m_fire.m_temperatureSource.setTemperature(temperature);
		uint32_t delay = m_fire.m_materialType.burnData->burnStageDuration / Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(delay, m_fire);
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Smouldering)
	{
		// Clear the event pointer so ~Fire doesn't try to cancel the event which is currently executing.
		m_fire.m_event.clearPointer();
		// Destroy m_fire by relasing the unique pointer in m_location.
		// Implicitly removes the influence of m_fire.m_temperatureSource.
		m_fire.m_location.m_area->m_fires.extinguish(m_fire);
	}
}
FireEvent::~FireEvent()
{
	m_fire.m_event.clearPointer();
}
Fire::Fire(Block& l, const MaterialType& mt) : m_location(l), m_materialType(mt), m_stage(FireStage::Smouldering), m_hasPeaked(false), m_temperatureSource(m_materialType.burnData->flameTemperature / Config::heatFractionForSmoulder, l)
{
	m_event.schedule(m_materialType.burnData->burnStageDuration, *this);
}
void HasFires::ignite(Block& block, const MaterialType& materialType)
{
	if(m_fires.contains(&block))
		assert(std::ranges::find(m_fires.at(&block), materialType, [](Fire& fire){ return fire.m_materialType; }) == m_fires.at(&block).end());
	m_fires[&block].emplace_back(block, materialType);
}
void HasFires::extinguish(Fire& fire)
{
	assert(m_fires.contains(&fire.m_location));
	fire.m_temperatureSource.unapply();
	m_fires.at(&fire.m_location).remove(fire);
}
