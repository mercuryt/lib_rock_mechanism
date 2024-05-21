#include "simulation.h"
#include "actor.h"
#include "animalSpecies.h"
#include "config.h"
#include "definitions.h"
#include "deserializationMemo.h"
#include "haul.h"
#include "simulation/hasActors.h"
#include "simulation/hasAreas.h"
#include "simulation/hasItems.h"
#include "threadedTask.h"
#include "util.h"
#include "drama/engine.h"
//#include "worldforge/world.h"
#include <cstdint>
#include <filesystem>
#include <functional>
Simulation::Simulation(std::wstring name, Step s) : 
       	m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this), m_deserializationMemo(*this), m_name(name), m_step(s)
{ 
	m_hourlyEvent.schedule(*this);
	m_path.append(L"save/"+name);
	m_hasItems = std::make_unique<SimulationHasItems>(*this);
	m_dramaEngine = std::make_unique<DramaEngine>(*this);
	m_hasActors = std::make_unique<SimulationHasActors>(*this);
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
	m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this), m_deserializationMemo(*this) 
{
	m_name = data["name"].get<std::wstring>();
	m_step = data["step"].get<Step>();
	//if(data["world"])
		//m_world = std::make_unique<World>(data["world"], deserializationMemo);
	m_hasFactions.load(data["factions"], m_deserializationMemo);
	m_hourlyEvent.schedule(*this, data["hourEventStart"].get<Step>());
	m_hasItems = std::make_unique<SimulationHasItems>(data["hasItems"], m_deserializationMemo, *this);
	m_hasActors = std::make_unique<SimulationHasActors>(data["hasActors"], m_deserializationMemo, *this);
	m_hasAreas = std::make_unique<SimulationHasAreas>(data["hasAreas"], m_deserializationMemo, *this);
}
void Simulation::doStep(uint16_t count)
{
	assert(count);
	bool locked = false;
	for(uint i = 0; i < count; ++i)
	{
		m_taskFutures.clear();
		m_threadedTaskEngine.readStep();
		m_hasAreas->readStep();
		// Wait for all the tasks to complete.
		for(auto& future : m_taskFutures)
			future.wait();
		// Aquire UI read mutex on first iteration, 
		if(!locked)
		{
			m_uiReadMutex.lock();
			locked = true;
		}
		m_hasAreas->writeStep();
		m_threadedTaskEngine.writeStep();
		// Do scheduled events last to avoid unexpected state changes in threaded task data between read and write.
		m_eventSchedule.execute(m_step);
		// Apply user input.
		m_inputQueue.flush();
		++m_step;
	}
	m_uiReadMutex.unlock();
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
	m_hasItems->clearAll();
	m_hasActors->clearAll();
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
void Simulation::fastForwardUntillActorIsAtDestination(Actor& actor, Block& destination)
{
	assert(*actor.m_canMove.getDestination() == destination);
	fastForwardUntillActorIsAt(actor, destination);
}
void Simulation::fastForwardUntillActorIsAt(Actor& actor, Block& destination)
{
	std::function<bool()> predicate = [&](){ return actor.m_location == &destination; };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorIsAdjacentToDestination(Actor& actor, Block& destination)
{
	assert(!actor.m_canMove.getPath().empty());
	[[maybe_unused]] Block* adjacentDestination = actor.m_canMove.getPath().back();
	assert(adjacentDestination != nullptr);
	if(actor.m_blocks.size() == 1)
		assert(adjacentDestination->isAdjacentToIncludingCornersAndEdges(destination));
	std::function<bool()> predicate = [&](){ return actor.isAdjacentTo(destination); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorIsAdjacentTo(Actor& actor, Block& block)
{
	std::function<bool()> predicate = [&](){ return actor.isAdjacentTo(block); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorIsAdjacentToHasShape(Actor& actor, HasShape& other)
{
	std::function<bool()> predicate = [&](){ return actor.isAdjacentTo(other); };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorHasNoDestination(Actor& actor)
{
	std::function<bool()> predicate = [&](){ return actor.m_canMove.getDestination() == nullptr; };
	fastForwardUntillPredicate(predicate);
}
void Simulation::fastForwardUntillActorHasEquipment(Actor& actor, Item& item)
{
	std::function<bool()> predicate = [&](){ return actor.m_equipmentSet.contains(item); };
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
Block& Simulation::getBlockForJsonQuery(const Json& data)
{
	auto x = data["x"].get<uint32_t>();
	auto y = data["y"].get<uint32_t>();
	auto z = data["z"].get<uint32_t>();
	auto id = data["area"].get<AreaId>();
	Area& area = m_hasAreas->getById(id);
	return area.getBlock(x, y, z);
}
