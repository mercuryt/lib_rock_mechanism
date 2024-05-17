#include "hasAreas.h"

#include "../simulation.h"
#include "../actor.h"
#include "../deserializationMemo.h"
#include "../drama/engine.h"

SimulationHasAreas::SimulationHasAreas(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation) : m_simulation(simulation)
{
	m_nextId = data["nextId"].get<AreaId>();
	for(const Json& areaId : data["areaIds"])
	{
		std::ifstream af(m_simulation.m_path/"area"/(std::to_string(areaId.get<AreaId>()) + ".json"));
		Json areaData = Json::parse(af);
		loadAreaFromJson(areaData, deserializationMemo);
	}
}
void SimulationHasAreas::readStep()
{
	for(Area& area : m_areas)
		area.readStep();
}
void SimulationHasAreas::writeStep()
{
	for(Area& area : m_areas)
		area.writeStep();
}
void SimulationHasAreas::incrementHour()
{
	for(Area& area : m_areas)
		area.updateClimate();
}
void SimulationHasAreas::save()
{
	for(Area& area : m_areas)
	{
		std::ofstream af(m_simulation.m_path/"area"/(std::to_string(area.m_id) + ".json"));
		af << area.toJson();
	}
}
Area& SimulationHasAreas::createArea(uint32_t x, uint32_t y, uint32_t z, bool createDrama)
{ 
	AreaId id = ++ m_nextId;
	Area& output = loadArea(id, L"unnamed area " + std::to_wstring(id), x, y, z);
	if(createDrama)
		m_simulation.m_dramaEngine->createArcsForArea(output);
	return output;
}
Area& SimulationHasAreas::loadArea(AreaId id, std::wstring name, uint32_t x, uint32_t y, uint32_t z)
{
	Area& area = m_areas.emplace_back(id, name, m_simulation, x, y, z); 
	m_areasById[id] = &area;
	return area;
}
void SimulationHasAreas::destroyArea(Area& area)
{
	m_simulation.m_dramaEngine->removeArcsForArea(area);
	for(Actor* actor : area.m_hasActors.getAll())
		actor->exit();
	m_areasById.erase(area.m_id);
	m_areas.remove(area);
}
Area& SimulationHasAreas::loadAreaFromJson(const Json& data, DeserializationMemo& deserializationMemo)
{
	return m_areas.emplace_back(data, deserializationMemo, m_simulation);
}
void SimulationHasAreas::clearAll()
{
	for(Area& area : m_areas)
		area.clearReservations();
	m_areas.clear();
}
void SimulationHasAreas::recordId(Area& area)
{
	m_areasById[area.m_id] = &area;
}
Json SimulationHasAreas::toJson() const
{
	Json areaIds = Json::array();
	for(const Area& area : m_areas)
		areaIds.push_back(area.m_id);
	return {{"areaIds", areaIds}, {"nextId", m_nextId}};
}
