#include "simulation.h"
#include "animalSpecies.h"
#include "config.h"
#include "definitions.h"
#include "deserializationMemo.h"
#include "haul.h"
#include "simulation/hasActors.h"
#include "simulation/hasAreas.h"
#include "simulation/hasItems.h"
#include "threadedTask.h"
#include "types.h"
#include "util.h"
#include "drama/engine.h"
//#include "worldforge/world.h"
#include <cstdint>
#include <filesystem>
#include <functional>
Simulation::Simulation(std::wstring name, Step s) : 
       	m_eventSchedule(*this, nullptr), m_threadedTaskEngine(*this), m_hourlyEvent(m_eventSchedule), m_deserializationMemo(*this), m_name(name), m_step(s)
{ 
	m_hourlyEvent.schedule(*this);
	m_path.append(L"save/"+name);
	m_dramaEngine = std::make_unique<DramaEngine>(*this);
	m_hasAreas = std::make_unique<SimulationHasAreas>(*this);
}
Simulation::Simulation(std::filesystem::path path) : Simulation(Json::parse(std::ifstream{path/"simulation.json"})) 
{ 
	m_path = path; 
	const Json& data = Json::parse(std::ifstream{m_path/"simulation.json"});
	for(const Json& areaId : data["areaIds"])
		m_hasAreas->loadAreaFromPath(areaId, m_deserializationMemo);
	//TODO: DramaEngine should probably be able to load before hasAreas.
	m_dramaEngine = std::make_unique<DramaEngine>(data["drama"], m_deserializationMemo, *this);
}
Simulation::Simulation(const Json& data) : 
	m_eventSchedule(*this, nullptr), m_threadedTaskEngine(*this), m_hourlyEvent(m_eventSchedule), m_deserializationMemo(*this) 
{
	m_name = data["name"].get<std::wstring>();
	m_step = data["step"].get<Step>();
	//if(data["world"])
		//m_world = std::make_unique<World>(data["world"], deserializationMemo);
	m_hasFactions.load(data["factions"], m_deserializationMemo);
	m_hourlyEvent.schedule(*this, data["hourEventStart"].get<Step>());
	m_hasAreas = std::make_unique<SimulationHasAreas>(data["hasAreas"], m_deserializationMemo, *this);
}
void Simulation::doStep(uint16_t count)
{
	assert(count);
	bool locked = false;
	for(uint i = 0; i < count; ++i)
	{
		// Aquire UI read mutex on first iteration, 
		if(!locked)
		{
			m_uiReadMutex.lock();
			locked = true;
		}
		m_threadedTaskEngine.doStep(*this, nullptr);
		m_hasAreas->doStep();
		m_eventSchedule.doStep(m_step);
		// Apply user input.
		//m_inputQueue.flush();
		++m_step;
	}
	m_uiReadMutex.unlock();
}
template<class Data, class Action>
void Simulation::parallelizeTask(Data& data, uint32_t stepSize, Action& task)
{
	auto start = data.begin();
	auto end = start + std::min(stepSize, data.size());
	while(start != end)
	{
		m_pool.push_task([&data, &task, start, end](){
			while(start != end)
				task(data[start++]);
		});
		start = end;
		end = std::min(data.size(), start + stepSize);
	}
	m_pool.wait_for_tasks();
}
template<class Data, class Action>
void Simulation::parallelizeTaskIndices(uint32_t size, uint32_t stepSize, Action& task)
{
	uint32_t start = 0;
	uint32_t end = start + std::min(stepSize, size);
	while(start != size)
	{
		m_pool.push_task([&task, start, end]() mutable {
			while(start != end)
				task(start++);
		});
		start = end;
		end = std::min(size, start + stepSize);
	}
	m_pool.wait_for_tasks();
}
void Simulation::incrementHour()
{
	m_hasAreas->incrementHour();
	m_hourlyEvent.schedule(*this);
}
void Simulation::save()
{
	std::filesystem::create_directory(m_path);
	std::ofstream f(m_path/"simulation.json");
	f << toJson();
	std::filesystem::create_directory(m_path/"area");
	m_hasAreas->save();
}
Faction& Simulation::createFaction(std::wstring name) { return m_hasFactions.createFaction(name); }
DateTime Simulation::getDateTime() const { return DateTime(m_step); }
Simulation::~Simulation()
{
	m_dramaEngine = nullptr;
	m_hasAreas->clearAll();
	m_hourlyEvent.maybeUnschedule();
	m_eventSchedule.m_data.clear();
	m_threadedTaskEngine.clear();
	m_pool.wait_for_tasks();
}
// Note: Does not handle fluids.
void Simulation::fastForward(Step steps)
{
	Step targetStep = m_step + steps;
	while(!m_eventSchedule.m_data.empty())
	{
		auto& pair = *m_eventSchedule.m_data.begin();
		if(pair.first <= targetStep)
		{
			m_step = pair.first;
			doStep();
		}
		else
			break;
	}
	m_step = targetStep + 1;
}
void Simulation::fastForwardUntill(DateTime dateTime)
{
	assert(dateTime.toSteps() > m_step);
	fastForward(dateTime.toSteps() - m_step);
}
void Simulation::fastForwardUntillActorIsAtDestination(Area& area, ActorIndex actor, BlockIndex destination)
{
	assert(area.m_actors.move_getDestination(actor) == destination);
	fastForwardUntillActorIsAt(area, actor, destination);
}
void Simulation::fastForwardUntillActorIsAt(Area& area, ActorIndex actor, BlockIndex destination)
{
	BlockIndex location = area.m_actors.getLocation(actor);
	std::function<bool()> predicate = [&](){ return location == destination; };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorIsAdjacentToDestination(Area& area, ActorIndex actor, BlockIndex destination)
{
	Actors& actors = area.m_actors;
	assert(!actors.move_getPath(actor).empty());
	BlockIndex adjacentDestination = actors.move_getPath(actor).back();
	assert(adjacentDestination != BLOCK_INDEX_MAX);
	if(actors.getBlocks(actor).size() == 1)
		assert(area.getBlocks().isAdjacentToIncludingCornersAndEdges(adjacentDestination, destination));
	std::function<bool()> predicate = [&](){ return actors.isAdjacentToLocation(actor, destination); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorIsAdjacentToLocation(Area& area, ActorIndex actor, BlockIndex block)
{
	std::function<bool()> predicate = [&](){ return area.m_actors.isAdjacentToLocation(actor, block); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorIsAdjacentToActor(Area& area, ActorIndex actor, ActorIndex other)
{
	std::function<bool()> predicate = [&](){ return area.m_actors.isAdjacentToActor(actor, other); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorIsAdjacentToItem(Area& area, ActorIndex actor, ItemIndex item)
{
	std::function<bool()> predicate = [&](){ return area.m_actors.isAdjacentToItem(actor, item); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorHasNoDestination(Area& area, ActorIndex actor)
{
	std::function<bool()> predicate = [&](){ return area.m_actors.move_getDestination(actor) == BLOCK_INDEX_MAX; };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorHasEquipment(Area& area, ActorIndex actor, ItemIndex item)
{
	Actors& actors = area.m_actors;
	std::function<bool()> predicate = [&](){ return actors.getEquipmentSet(actor).contains(item); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillPredicate(std::function<bool()> predicate, uint32_t minutes)
{
	assert(!predicate());
	[[maybe_unused]] Step lastStep = m_step + (minutes * Config::stepsPerMinute);
	while(!m_eventSchedule.m_data.empty())
	{
		if(m_threadedTaskEngine.count() == 0)
		{
			auto& pair = *m_eventSchedule.m_data.begin();
			m_step = pair.first;
		}
		assert(m_step <= lastStep);
		doStep();
		if(predicate())
			break;
	}
}
void Simulation::fastForwardUntillNextEvent()
{
	auto& pair = *m_eventSchedule.m_data.begin();
	fastForward(pair.first - m_step);
}
/*
Json Simulation::toJson() const
{
	Json output;
	output["name"] = m_name;
	//output["world"] = m_world;
	output["step"] = m_step;
	output["hasActors"] = m_hasActors->toJson();
	output["hasItems"] = m_hasItems->toJson();
	output["hasAreas"] = m_hasAreas->toJson();
	output["factions"] = m_hasFactions.toJson();
	output["hourEventStart"] = m_hourlyEvent.getStartStep();
	output["drama"] = m_dramaEngine->toJson();
	return output;
}
*/
