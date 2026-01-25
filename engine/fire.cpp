#include "fire.h"
#include "area/area.h"
#include "simulation/simulation.h"
#include "definitions/materialType.h"
#include "space/space.h"
#include "numericTypes/types.h"
void Fire::nextPhase(Area& area)
{
	if(!m_hasPeaked &&m_stage == FireStage::Smouldering)
	{
		m_stage = FireStage::Burning;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_materialType) * Config::heatFractionForBurn;
		m_temperatureSource.setTemperature(area, temperature);
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + MaterialType::getBurnStageDuration(m_materialType), *this);
	}
	else if(!m_hasPeaked && m_stage == FireStage::Burning)
	{
		m_stage = FireStage::Flaming;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_materialType);
		m_temperatureSource.setTemperature(area, temperature);
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + MaterialType::getFlameStageDuration(m_materialType), *this);
	}
	else if(m_stage == FireStage::Flaming)
	{
		m_hasPeaked = true;
		m_stage = FireStage::Burning;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_materialType) * Config::heatFractionForBurn;
		m_temperatureSource.setTemperature(area, temperature);
		Step delay = MaterialType::getBurnStageDuration(m_materialType) * Config::fireRampDownPhaseDurationFraction;
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + delay, *this);
		Space& space = area.getSpace();
		if(space.solid_isAny(m_location) && space.solid_get(m_location) == m_materialType)
		{
			space.solid_setNot(m_location);
			//TODO: create debris / wreckage?
		}
	}
	else if(m_hasPeaked && m_stage == FireStage::Burning)
	{
		m_stage = FireStage::Smouldering;
		TemperatureDelta temperature = MaterialType::getFlameTemperature(m_materialType) * Config::heatFractionForSmoulder;
		m_temperatureSource.setTemperature(area, temperature);
		Step delay = MaterialType::getBurnStageDuration(m_materialType) * Config::fireRampDownPhaseDurationFraction;
		area.m_fires.scheduleNextPhase(area.m_simulation.m_step + delay, *this);
	}
	else if(m_hasPeaked && m_stage == FireStage::Smouldering)
	{
		// Clear the event pointer so ~Fire doesn't try to cancel the event which is currently executing.
		// Implicitly removes the influence of m_temperatureSource.
		area.m_fires.extinguish(area, *this);
	}
}
bool Fire::operator==(const Fire& fire) const { return &fire == this; }
FireDelta Fire::createDelta() const  { return { m_location, m_materialType}; }
// Fire.
Fire::Fire(Area& area, const Point3D& l, const MaterialTypeId& mt, bool hasPeaked, FireStage stage) :
	m_temperatureSource(area, MaterialType::getFlameTemperature(mt) * Config::heatFractionForSmoulder, l),
	m_location(l), m_materialType(mt), m_stage(stage), m_hasPeaked(hasPeaked)
{ }
void AreaHasFires::doStep(const Step& step, Area& area)
{
	m_deltas.sortDescending();
	if(m_deltas.empty() || m_deltas.back().first != step)
		return;
	// Move copy deltas for step so we can popBack before adding any new deltas via nextPhase.
	auto deltasForStep = std::move(m_deltas.back().second);
	m_deltas.popBack();
	// Deltas may have become invalidated due to the fire being extinguished, missing fires are ignored.
	for(const FireDelta& delta : deltasForStep)
	{
		auto foundLocation = m_fires.find(delta.location);
		if(foundLocation != m_fires.end())
		{
			auto foundFire = foundLocation->second.find(delta.materialType);
			if(foundFire != foundLocation->second.end())
				foundFire->second.nextPhase(area);
		}
	}
}
void AreaHasFires::scheduleNextPhase(const Step& step, const Fire& fire)
{
	m_deltas.getOrCreate(step).insert(fire.createDelta());
}
void AreaHasFires::ignite(Area& area, const Point3D& point, const MaterialTypeId& materialType)
{
	if(m_fires.contains(point))
		assert(!m_fires.at(point).contains(materialType));
	m_fires[point].emplace(materialType, area, point, materialType);
	scheduleNextPhase(area.m_simulation.m_step + MaterialType::getBurnStageDuration(materialType), m_fires[point].back().second);
	// Temperature delta is applied to space in Fire constructor.
	Space& space = area.getSpace();
	if(!space.fire_exists(point))
		space.fire_setPointer(point, &m_fires.at(point));
}
void AreaHasFires::extinguish(Area& area, Fire& fire)
{
	assert(m_fires.contains(fire.m_location));
	Point3D point = fire.m_location;
	fire.m_temperatureSource.unapply(area);
	m_fires.at(point).erase(fire.m_materialType);
	if(m_fires.at(point).empty())
		area.getSpace().fire_clearPointer(point);
}
void AreaHasFires::load(Area& area, const Json& data, DeserializationMemo&)
{
	for(const auto& pair : data["fires"])
		for(const Json& fireData : pair[1])
		{
			Point3D point = fireData["location"].get<Point3D>();
			MaterialTypeId materialType = fireData["materialType"].get<MaterialTypeId>();
			m_fires[point].emplace(materialType, area, point, materialType, fireData["hasPeaked"].get<bool>(), fireData["stage"].get<FireStage>());
		}
	data["deltas"].get_to(m_deltas);
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
	Json data{{"fires", Json::array()}, {"deltas", m_deltas}};
	for(auto& [point, fires] : m_fires)
	{
		data["fires"].push_back(Json{point, Json::array()});
		for(auto& [materialType, fire] : fires)
		{
			assert(materialType == fire.m_materialType);
			Json fireData{{"location", fire.m_location}, {"materialType", fire.m_materialType}, {"hasPeaked", fire.m_hasPeaked}, {"stage", fire.m_stage}};
			data["fires"].back()[1].push_back(fireData);
		}
	}
	return data;
}

bool AreaHasFires::containsFireAt(Fire& fire, const Point3D& point) const { return m_fires.at(point).contains(fire.m_materialType); }
bool AreaHasFires::containsDeltaFor(Fire& fire) const
{
	const FireDelta delta = fire.createDelta();
	for(const auto& [step, deltas] : m_deltas)
		if(deltas.contains(delta))
			return true;
	return false;
}
bool AreaHasFires::containsDeltas() const
{
	return !m_deltas.empty();
}