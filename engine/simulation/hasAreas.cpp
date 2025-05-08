#include "hasAreas.h"

#include "../simulation/simulation.h"
#include "../deserializationMemo.h"
#include "../drama/engine.h"
#include "../area/area.h"
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
	for(auto& pair : m_areas)
		pair.second->doStep();
}
void SimulationHasAreas::incrementHour()
{
	for(auto& pair : m_areas)
		pair.second->updateClimate();
}
void SimulationHasAreas::save()
{
	for(auto& pair : m_areas)
	{
		std::ofstream af(m_simulation.m_path/"area"/(std::to_string(pair.second->m_id.get()) + ".json"));
		af << pair.second->toJson();
	}
}
Area& SimulationHasAreas::createArea(const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z, bool createDrama)
{
	AreaId id = ++m_nextId;
	Area& output = loadArea(id, "unnamed area " + std::to_string(id.get()), x, y, z);
	if(createDrama)
		m_simulation.m_dramaEngine->createArcsForArea(output);
	return output;
}
Area& SimulationHasAreas::createArea(uint x, uint y, uint z, bool createDrama)
{
	return createArea(DistanceInBlocks::create(x), DistanceInBlocks::create(y), DistanceInBlocks::create(z), createDrama);
}
Area& SimulationHasAreas::loadArea(const AreaId& id, std::string name, const DistanceInBlocks& x, const DistanceInBlocks& y, const DistanceInBlocks& z)
{
	Area& area = m_areas.emplace(id, id, name, m_simulation, x, y, z);
	m_areasById.insert(id, &area);
	return area;
}
void SimulationHasAreas::destroyArea(Area& area)
{
	m_simulation.m_dramaEngine->removeArcsForArea(area);
	Actors& actors = area.getActors();
	for(ActorIndex actor : actors.getAll())
		actors.location_clear(actor);
	m_areasById.erase(area.m_id);
	m_areas.erase(area.m_id);
}
Area& SimulationHasAreas::loadAreaFromJson(const Json& data, DeserializationMemo& deserializationMemo)
{
	const AreaId id = AreaId::create(data["id"].get<uint>());
	return m_areas.insert(id, std::make_unique<Area>(data, deserializationMemo, m_simulation));
}
Area& SimulationHasAreas::loadAreaFromPath(const AreaId& id, DeserializationMemo& deserializationMemo)
{
	std::ifstream af(m_simulation.m_path/"area"/(std::to_string(id.get()) + ".json"));
	Json areaData = Json::parse(af);
	return loadAreaFromJson(areaData, deserializationMemo);
}
void SimulationHasAreas::clearAll()
{
	for(auto& pair : m_areas)
		pair.second->clearReservations();
	m_areas.clear();
}
void SimulationHasAreas::recordId(Area& area)
{
	m_areasById.insert(area.m_id, &area);
}
Step SimulationHasAreas::getNextEventStep() const
{
	Step output;
	for(const auto& pair : m_areas)
	{
		Step step = pair.second->m_eventSchedule.getNextEventStep();
		if(output.empty() || step < output)
			output = step;
	}
	return output;
}
Step SimulationHasAreas::getNextStepToSimulate() const
{
	Step output;
	for(const auto& pair : m_areas)
	{
		const Area& area = *pair.second;
		if(!area.m_hasTerrainFacades.empty() || !area.m_hasFluidGroups.getUnstable().empty() || !area.m_threadedTaskEngine.empty())
			return m_simulation.m_step;
		Step step = area.m_eventSchedule.getNextEventStep();
		if(output.empty() || step < output)
			output = step;
	}
	return output;
}
Json SimulationHasAreas::toJson() const
{
	Json areaIds = Json::array();
	for(const auto& pair : m_areas)
		areaIds.push_back(pair.second->m_id);
	return {{"areaIds", areaIds}, {"nextId", m_nextId}};
}
