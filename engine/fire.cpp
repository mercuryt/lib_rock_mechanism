#include "fire.h"
#include "block.h"
#include "area.h"
FireEvent::FireEvent(Step delay, Fire& f, Step start) : ScheduledEvent(f.m_location.m_area->m_simulation, delay, start), m_fire(f) {}
void FireEvent::execute()
{
	if(!m_fire.m_hasPeaked &&m_fire.m_stage == FireStage::Smouldering)
	{
		m_fire.m_stage = FireStage::Burning;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature * Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(temperature);
		m_fire.m_event.schedule(m_fire.m_materialType.burnData->burnStageDuration, m_fire);
	}
	else if(!m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burning)
	{
		m_fire.m_stage = FireStage::Flaming;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature;
		m_fire.m_temperatureSource.setTemperature(temperature);
		m_fire.m_event.schedule(m_fire.m_materialType.burnData->flameStageDuration, m_fire);
	}
	else if(m_fire.m_stage == FireStage::Flaming)
	{
		m_fire.m_hasPeaked = true;
		m_fire.m_stage = FireStage::Burning;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature * Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(temperature);
		uint32_t delay = m_fire.m_materialType.burnData->burnStageDuration * Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(delay, m_fire);
		if(m_fire.m_location.isSolid() && m_fire.m_location.getSolidMaterial() == m_fire.m_materialType)
		{
			m_fire.m_location.setNotSolid();
			//TODO: create debris / wreckage?
		}
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burning)
	{
		m_fire.m_stage = FireStage::Smouldering;
		int32_t temperature = m_fire.m_materialType.burnData->flameTemperature * Config::heatFractionForSmoulder;
		m_fire.m_temperatureSource.setTemperature(temperature);
		uint32_t delay = m_fire.m_materialType.burnData->burnStageDuration * Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(delay, m_fire);
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Smouldering)
	{
		// Clear the event pointer so ~Fire doesn't try to cancel the event which is currently executing.
		//m_fire.m_event.clearPointer();
		// Destroy m_fire by relasing the unique pointer in m_location.
		// Implicitly removes the influence of m_fire.m_temperatureSource.
		m_fire.m_location.m_area->m_fires.extinguish(m_fire);
	}
}
void FireEvent::clearReferences() { m_fire.m_event.clearPointer(); }
// Fire.
Fire::Fire(Block& l, const MaterialType& mt, bool hasPeaked, FireStage stage, Step start) : m_location(l), m_materialType(mt), m_event(l.m_area->m_simulation.m_eventSchedule), m_stage(stage), m_hasPeaked(hasPeaked), m_temperatureSource(m_materialType.burnData->flameTemperature * Config::heatFractionForSmoulder, l)
{
	m_event.schedule(m_materialType.burnData->burnStageDuration, *this, start);
}
void AreaHasFires::ignite(Block& block, const MaterialType& materialType)
{
	if(m_fires.contains(&block))
		assert(!m_fires.at(&block).contains(&materialType));
	m_fires[&block].try_emplace(&materialType, block, materialType);
	if(block.m_fires == nullptr)
		block.m_fires = &m_fires.at(&block);
}
void AreaHasFires::extinguish(Fire& fire)
{
	assert(m_fires.contains(&fire.m_location));
	Block& block = fire.m_location;
	fire.m_temperatureSource.unapply();
	m_fires.at(&block).erase(&fire.m_materialType);
	if(m_fires.at(&block).empty())
		block.m_fires = nullptr;
}
void AreaHasFires::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	for(const Json& fireData : data)
	{
		Block& block = deserializationMemo.m_simulation.getBlockForJsonQuery(fireData["location"]);
		const MaterialType& materialType = *fireData["materialType"].get<const MaterialType*>();
		m_fires[&block].try_emplace(&materialType, block, materialType, fireData["hasPeaked"].get<bool>(), fireData["stage"].get<FireStage>(), fireData["start"].get<Step>());
	}
}
Json AreaHasFires::toJson() const
{
	Json data;
	for(auto& [blockReference, fires] : m_fires)
	{
		data.push_back(Json{blockReference, Json::array()});
		for(auto& [materialType, fire] : fires)
		{
			Json fireData{{"location", &fire.m_location}, {"materialType", fire.m_materialType}, {"hasPeaked", fire.m_hasPeaked}, {"stage", fire.m_stage}, {"start", fire.m_event.getStartStep()}};
			data.back()[1].push_back(Json{materialType, fireData});
		}
	}
	return data;
}
