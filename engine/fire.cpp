#include "fire.h"
#include "area/area.h"
#include "simulation/simulation.h"
#include "definitions/materialType.h"
#include "space/space.h"
#include "numericTypes/types.h"
FireEvent::FireEvent(Area& area, const Step& delay, Fire& f, const Step start) :
       	ScheduledEvent(area.m_simulation, delay, start), m_fire(f) { }
void FireEvent::execute(Simulation&, Area* area)
{
	if(!m_fire.m_hasPeaked &&m_fire.m_stage == FireStage::Smouldering)
	{
		m_fire.m_stage = FireStage::Burning;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_solid) * Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		m_fire.m_event.schedule(*area, MaterialType::getBurnStageDuration(m_fire.m_solid), m_fire);
	}
	else if(!m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burning)
	{
		m_fire.m_stage = FireStage::Flaming;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_solid);
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		m_fire.m_event.schedule(*area, MaterialType::getFlameStageDuration(m_fire.m_solid), m_fire);
	}
	else if(m_fire.m_stage == FireStage::Flaming)
	{
		m_fire.m_hasPeaked = true;
		m_fire.m_stage = FireStage::Burning;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_solid) * Config::heatFractionForBurn;
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		Step delay = MaterialType::getBurnStageDuration(m_fire.m_solid) * Config::fireRampDownPhaseDurationFraction;
		m_fire.m_event.schedule(*area, delay, m_fire);
		Space& space = area->getSpace();
		if(space.solid_is(m_fire.m_location) && space.solid_get(m_fire.m_location) == m_fire.m_solid)
		{
			space.solid_setNot(m_fire.m_location);
			//TODO: create debris / wreckage?
		}
	}
	else if(m_fire.m_hasPeaked && m_fire.m_stage == FireStage::Burning)
	{
		m_fire.m_stage = FireStage::Smouldering;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_fire.m_solid) * Config::heatFractionForSmoulder;
		m_fire.m_temperatureSource.setTemperature(*area, temperature);
		Step delay = MaterialType::getBurnStageDuration(m_fire.m_solid) * Config::fireRampDownPhaseDurationFraction;
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
Fire::Fire(Area& a, const Point3D& l, const MaterialTypeId& mt, bool hasPeaked, FireStage stage, const Step start) :
	m_temperatureSource(a, MaterialType::getFlameTemperature(mt) * Config::heatFractionForSmoulder, l),
	m_event(a.m_eventSchedule), m_location(l), m_solid(mt), m_stage(stage), m_hasPeaked(hasPeaked)
{
	m_event.schedule(a, MaterialType::getBurnStageDuration(m_solid), *this, start);
}
void AreaHasFires::ignite(const Point3D& point, const MaterialTypeId& materialType)
{
	if(m_fires.contains(point))
		assert(!m_fires.at(point).contains(materialType));
	m_fires[point].emplace(materialType, m_area, point, materialType);
	Space& space = m_area.getSpace();
	if(!space.fire_exists(point))
		space.fire_setPointer(point, &m_fires.at(point));
}
void AreaHasFires::extinguish(Fire& fire)
{
	assert(m_fires.contains(fire.m_location));
	Point3D point = fire.m_location;
	fire.m_temperatureSource.unapply(m_area);
	m_fires.at(point).erase(fire.m_solid);
	if(m_fires.at(point).empty())
		m_area.getSpace().fire_clearPointer(point);
}
void AreaHasFires::load(const Json& data, DeserializationMemo&)
{
	for(const auto& pair : data)
		for(const Json& fireData : pair[1])
		{
			Point3D point = fireData["location"].get<Point3D>();
			MaterialTypeId materialType = fireData["materialType"].get<MaterialTypeId>();
			m_fires[point].emplace(materialType, m_area, point, materialType, fireData["hasPeaked"].get<bool>(), fireData["stage"].get<FireStage>(), fireData["start"].get<Step>());
		}
}
Fire& AreaHasFires::at(const Point3D& point, const MaterialTypeId& materialType)
{
	assert(m_fires.contains(point));
	assert(m_fires.at(point).contains(materialType));
	return m_fires.at(point)[materialType];
}
bool AreaHasFires::contains(const Point3D& point, const MaterialTypeId& materialType)
{
	if(!m_fires.contains(point))
		return false;
	return m_fires.at(point).contains(materialType);
}
Json AreaHasFires::toJson() const
{
	Json data;
	for(auto& [point, fires] : m_fires)
	{
		data.push_back(Json{point, Json::array()});
		for(auto& [materialType, fire] : fires)
		{
			assert(materialType == fire->m_solid);
			Json fireData{{"location", fire->m_location}, {"materialType", fire->m_solid}, {"hasPeaked", fire->m_hasPeaked}, {"stage", fire->m_stage}, {"start", fire->m_event.getStartStep()}};
			data.back()[1].push_back(fireData);
		}
	}
	return data;
}
