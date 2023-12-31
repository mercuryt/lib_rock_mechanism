#include "simulation.h"
#include "animalSpecies.h"
#include "area.h"
#include "config.h"
#include "definitions.h"
#include "deserializationMemo.h"
#include "haul.h"
#include "threadedTask.h"
#include <cstdint>
#include <functional>
Simulation::Simulation(DateTime n, Step s) :  m_nextAreaId(1), m_step(s), m_now(n), m_nextActorId(1), m_nextItemId(1), m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this)
{ 
	m_hourlyEvent.schedule(*this);
}
Simulation::Simulation(const Json& data, std::filesystem::path path) : m_step(data["step"].get<Step>()), m_now(data["now"]), 
	m_nextActorId(data["nextActorId"].get<Step>()), m_nextItemId(data["nextItemId"].get<Step>()),
	m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this) 
{ 
	m_hourlyEvent.schedule(*this, data["hourlyEventStart"].get<Step>());
	DeserializationMemo deserializationMemo(*this);
	for(const Json& areaId : data["areaIds"])
	{
		std::ifstream af(path/"area"/(std::to_string(areaId.get<AreaId>()) + ".json"));
		Json areaData = Json::parse(af);
		m_areas.emplace_back(areaData, deserializationMemo, *this);
	}
}
void Simulation::doStep()
{
	m_taskFutures.clear();
	m_threadedTaskEngine.readStep();
	for(Area& area : m_areas)
		area.readStep();
	// Wait for all the tasks to complete.
	for(auto& future : m_taskFutures)
		future.wait();
	// Aquire UI read mutex.
	std::scoped_lock lock(m_uiReadMutex);
	for(Area& area : m_areas)
		area.writeStep();
	m_threadedTaskEngine.writeStep();
	// Do scheduled events last to avoid unexpected state changes in threaded task data between read and write.
	m_eventSchedule.execute(m_step);
	// Apply user input.
	m_inputQueue.flush();
	++m_step;
}
void Simulation::incrementHour()
{
	m_now.hour++;
	if(m_now.hour > Config::hoursPerDay)
	{
		m_now.hour = 1;
		m_now.day++;
		if(m_now.day > Config::daysPerYear)
		{
			m_now.day = 1;
			m_now.year++;
		}
		for(Area& area : m_areas)
			area.setDateTime(m_now);
	}
	m_hourlyEvent.schedule(*this);
}
void Simulation::save(std::filesystem::path path)
{
	std::ofstream f(path/"simulation.json");
	f << toJson();
	for(Area& area : m_areas)
	{
		std::ofstream af(path/"area"/(std::to_string(area.m_id) + ".json"));
		af << area.toJson();
	}
}
Area& Simulation::createArea(uint32_t x, uint32_t y, uint32_t z)
{ 
	AreaId id = m_nextAreaId++;
	return loadArea(id, x, y, z);
}
Area& Simulation::loadArea(AreaId id, uint32_t x, uint32_t y, uint32_t z)
{
	Area& area = m_areas.emplace_back(id, *this, x, y, z); 
	m_areasById[id] = &area;
	return area;
}
Actor& Simulation::createActor(const AnimalSpecies& species, Block& block, Percent percentGrown)
{
	Attributes attributes(species, percentGrown);
	std::wstring name(species.name.begin(), species.name.end());
	name.append(std::to_wstring(m_nextActorId));
	auto [iter, emplaced] = m_actors.try_emplace(
		m_nextActorId,
		*this,
		m_nextActorId,
		name,
		species,
		percentGrown,
		nullptr,
		attributes
	);	
	assert(emplaced);
	Actor& output = iter->second;
	output.setLocation(block);
	return output;
}
// Nongeneric
// No name or id.
Item& Simulation::createItemNongeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, CraftJob* cj)
{
	const uint32_t id = ++ m_nextItemId;
	return loadItemNongeneric(id, itemType, materialType, quality, percentWear, L"", cj);
}
// Generic
// No id.
Item& Simulation::createItemGeneric(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj)
{

	const uint32_t id = ++ m_nextItemId;
	return loadItemGeneric(id, itemType, materialType, quantity, cj);
}
// Id.
Item& Simulation::loadItemGeneric(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj)
{
	if(m_nextItemId <= id) m_nextItemId = id + 1;
	auto [iter, emplaced] = m_items.try_emplace(id, *this, id, itemType, materialType, quantity, cj);
	assert(emplaced);
	return iter->second;
}
Item& Simulation::loadItemNongeneric(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, std::wstring name, CraftJob* cj)
{
	if(m_nextItemId <= id) m_nextItemId = id + 1;
	auto [iter, emplaced] = m_items.try_emplace(id, *this, id, itemType, materialType, quality, percentWear, cj);
	assert(emplaced);
	Item& item = iter->second;
	item.m_name = name;
	return item;
}
void Simulation::destroyItem(Item& item)
{
	item.m_dataLocation->erase(item.m_iterator);
}
void Simulation::destroyArea(Area& area)
{
	for(Actor* actor : area.m_hasActors.getAll())
		actor->exit();
	m_areasById.erase(area.m_id);
	m_areas.remove(area);
}
Simulation::~Simulation()
{
	m_items.clear();
	m_actors.clear();
	m_areas.clear();
	m_hourlyEvent.maybeUnschedule();
	m_eventSchedule.m_data.clear();
	m_threadedTaskEngine.clear();
	m_pool.wait_for_tasks();
}
void Simulation::setDateTime(DateTime now)
{
	m_now = now;
	m_hourlyEvent.unschedule();
	m_hourlyEvent.schedule(*this);
	for(Area& area : m_areas)
		area.setDateTime(now);
}
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
	Block* adjacentDestination = actor.m_canMove.getPath().back();
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
void Simulation::fastForwardUntillPredicate(std::function<bool()> predicate, uint32_t minutes)
{
	assert(!predicate());
	Step lastStep = m_step + (minutes * Config::stepsPerMinute);
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
Json Simulation::toJson() const
{
	Json output;
	output["step"] = m_step;
	output["now"] = m_now.toJson();
	output["nextActorId"] = m_nextActorId;
	output["nextItemId"] = m_nextItemId;
	return output;
}
Area& Simulation::loadAreaFromJson(const Json& data)
{
	//TODO: embed data in structure with factions to allow json pointer access.
	DeserializationMemo deserializationMemo(*this);
	m_areas.emplace_back(data, deserializationMemo, *this);
	m_areasById[m_areas.back().m_id] = &m_areas.back();
	return m_areas.back();
}
Item& Simulation::loadItemFromJson(const Json& data, DeserializationMemo& deserializationMemo)
{
	auto id = data["id"].get<ItemId>();
	m_items.try_emplace(id, data, deserializationMemo, id);
	return m_items.at(id);
}
Actor& Simulation::loadActorFromJson(const Json& data, DeserializationMemo& deserializationMemo)
{
	auto id = data["id"].get<ActorId>();
	m_items.try_emplace(id, data, deserializationMemo, id);
	return m_actors.at(id);
}
Block& Simulation::getBlockForJsonQuery(const Json& data)
{
	auto x = data["x"].get<uint32_t>();
	auto y = data["y"].get<uint32_t>();
	auto z = data["z"].get<uint32_t>();
	auto id = data["area"].get<AreaId>();
	assert(m_areasById.contains(id));
	Area& area = *m_areasById.at(id);
	return area.getBlock(x, y, z);
}
Actor& Simulation::getActorById(ActorId id) 
{ 
	if(!m_actors.contains(id))
	{
		//TODO: Load from world DB.
		assert(false);
	}
	else
		return m_actors.at(id); 
} 
Item& Simulation::getItemById(ItemId id) 
{
	if(!m_items.contains(id))
	{
		//TODO: Load from world DB.
		assert(false);
	}
	else
		return m_items.at(id); 
} 
