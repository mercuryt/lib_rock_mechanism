#include "hasAreas.h"

#include "../simulation.h"
#include "../deserializationMemo.h"
#include "../drama/engine.h"
#include "../area.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../items/items.h"
#include "../plants.h"
#include "types.h"

SimulationHasAreas::SimulationHasAreas(const Json& data, DeserializationMemo&, Simulation& simulation) : m_simulation(simulation)
{
	m_nextId = data["nextId"].get<AreaId>();
}
void SimulationHasAreas::doStep()
{
	for(Area& area : m_areas)
		area.doStep();
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
		std::ofstream af(m_simulation.m_path/"area"/(std::to_string(area.m_id.get()) + ".json"));
		af << area.toJson();
	}
}
Area& SimulationHasAreas::createArea(DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z, bool createDrama)
{ 
	AreaId id = ++m_nextId;
	Area& output = loadArea(id, L"unnamed area " + std::to_wstring(id.get()), x, y, z);
	if(createDrama)
		m_simulation.m_dramaEngine->createArcsForArea(output);
	return output;
}
Area& SimulationHasAreas::createArea(uint x, uint y, uint z, bool createDrama)
{
	return createArea(DistanceInBlocks::create(x), DistanceInBlocks::create(y), DistanceInBlocks::create(z), createDrama);
}
Area& SimulationHasAreas::loadArea(AreaId id, std::wstring name, DistanceInBlocks x, DistanceInBlocks y, DistanceInBlocks z)
{
	Area& area = m_areas.emplace_back(id, name, m_simulation, x, y, z); 
	m_areasById.insert(id, &area);
	return area;
}
void SimulationHasAreas::destroyArea(Area& area)
{
	m_simulation.m_dramaEngine->removeArcsForArea(area);
	Actors& actors = area.getActors();
	for(ActorIndex actor : actors.getAll())
		actors.exit(actor);
	m_areasById.erase(area.m_id);
	m_areas.remove(area);
}
Area& SimulationHasAreas::loadAreaFromJson(const Json& data, DeserializationMemo& deserializationMemo)
{
	return m_areas.emplace_back(data, deserializationMemo, m_simulation);
}
Area& SimulationHasAreas::loadAreaFromPath(AreaId id, DeserializationMemo& deserializationMemo)
{
	std::ifstream af(m_simulation.m_path/"area"/(std::to_string(id.get()) + ".json"));
	Json areaData = Json::parse(af);
	return loadAreaFromJson(areaData, deserializationMemo);
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
