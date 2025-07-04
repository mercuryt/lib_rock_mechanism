#include "fire.h"
#include "area/area.h"
#include "simulation/simulation.h"
#include "definitions/materialType.h"
#include "blocks/blocks.h"
#include "numericTypes/types.h"
FireEvent::FireEvent(Area& area, const Step& delay, Fire& f, const Step start) :
       	ScheduledEvent(area.m_simulation, delay, start), m_fire(f) { }
void FireEvent::execute(Simulation&, Area* area)
{
	if(!m_fire.m_hasPeaked &&m_fire.m_stage == FireStage::Smouldering)
	{
		m_fire.m_stage = FireStage::Burning;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_materialType) * Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		m_fire.m_event.schedule(*area, MaterialType::getBurnStageDuration(m_fire.m_materialType), m_fire);
	}
	else if(!m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burning)
	{
		m_fire.m_stage = FireStage::Flaming;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_materialType);
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		m_fire.m_event.schedule(*area, MaterialType::getFlameStageDuration(m_fire.m_materialType), m_fire);
	}
	else if(m_fire.m_stage == FireStage::Flaming)
	{
		m_fire.m_hasPeaked = true;
		m_fire.m_stage = FireStage::Burning;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_materialType) * Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		Step delay = MaterialType::getBurnStageDuration(m_fire.m_materialType) * Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(*area, delay, m_fire);
		Blocks& blocks = area->getBlocks();
		if(blocks.solid_is(m_fire.m_location) && blocks.solid_get(m_fire.m_location) == m_fire.m_materialType)
		{
			blocks.solid_setNot(m_fire.m_location);
			//TODO: create debris / wreckage?
		}
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burning)
	{
		m_fire.m_stage = FireStage::Smouldering;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_materialType) * Config::heatFractionForSmoulder;
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		Step delay = MaterialType::getBurnStageDuration(m_fire.m_materialType) * Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(*area, delay, m_fire);
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Smouldering)
	{
		// Clear the event pointer so ~Fire doesn't try to cancel the event which is currently executing.
		//m_fire.m_event.clearPointer();
		// Destroy m_fire by relasing the unique pointer in m_location.
		// Implicitly removes the influence of m_fire.m_temperatureSource.
		area->m_fires.extinguish(m_fire);
	}
}
void FireEvent::clearReferences(Simulation&, Area*) { m_fire.m_event.clearPointer(); }
// Fire.
Fire::Fire(Area& a, const BlockIndex& l, const MaterialTypeId& mt, bool hasPeaked, FireStage stage, const Step start) :
	m_temperatureSource(a, MaterialType::getFlameTemperature(mt) * Config::heatFractionForSmoulder, l),
	m_event(a.m_eventSchedule), m_location(l), m_materialType(mt), m_stage(stage), m_hasPeaked(hasPeaked)
{
	m_event.schedule(a, MaterialType::getBurnStageDuration(m_materialType), *this, start);
}
void AreaHasFires::ignite(const BlockIndex& block, const MaterialTypeId& materialType)
{
	if(m_fires.contains(block))
		assert(!m_fires.at(block).contains(materialType));
	m_fires[block].emplace(materialType, m_area, block, materialType);
	Blocks& blocks = m_area.getBlocks();
	if(!blocks.fire_exists(block))
		blocks.fire_setPointer(block, &m_fires.at(block));
}
void AreaHasFires::extinguish(Fire& fire)
{
	assert(m_fires.contains(fire.m_location));
	BlockIndex block = fire.m_location;
	fire.m_temperatureSource.unapply(m_area);
	m_fires.at(block).erase(fire.m_materialType);
	if(m_fires.at(block).empty())
		m_area.getBlocks().fire_clearPointer(block);
}
void AreaHasFires::load(const Json& data, DeserializationMemo&)
{
	for(const auto& pair : data)
		for(const Json& fireData : pair[1])
		{
			BlockIndex block = fireData["location"].get<BlockIndex>();
			MaterialTypeId materialType = fireData["materialType"].get<MaterialTypeId>();
			m_fires[block].emplace(materialType, m_area, block, materialType, fireData["hasPeaked"].get<bool>(), fireData["stage"].get<FireStage>(), fireData["start"].get<Step>());
		}
}
Fire& AreaHasFires::at(const BlockIndex& block, const MaterialTypeId& materialType)
{
	assert(m_fires.contains(block));
	assert(m_fires.at(block).contains(materialType));
	return m_fires.at(block)[materialType];
}
bool AreaHasFires::contains(const BlockIndex& block, const MaterialTypeId& materialType)
{
	if(!m_fires.contains(block))
		return false;
	return m_fires.at(block).contains(materialType);
}
Json AreaHasFires::toJson() const
{
	Json data;
	for(auto& [blockReference, fires] : m_fires)
	{
		data.push_back(Json{blockReference, Json::array()});
		for(auto& [materialType, fire] : fires)
		{
			assert(materialType == fire->m_materialType);
			Json fireData{{"location", fire->m_location}, {"materialType", fire->m_materialType}, {"hasPeaked", fire->m_hasPeaked}, {"stage", fire->m_stage}, {"start", fire->m_event.getStartStep()}};
			data.back()[1].push_back(fireData);
		}
	}
	return data;
}
