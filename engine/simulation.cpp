#include "simulation.h"
#include "actor.h"
#include "animalSpecies.h"
#include "area.h"
#include "config.h"
#include "definitions.h"
#include "deserializationMemo.h"
#include "haul.h"
#include "threadedTask.h"
#include "util.h"
#include "drama/engine.h"
//#include "worldforge/world.h"
#include <cstdint>
#include <filesystem>
#include <functional>
Simulation::Simulation(std::wstring name, DateTime n, Step s) :  m_nextAreaId(1), m_deserializationMemo(*this), m_name(name), m_step(s), m_now(n), m_nextActorId(1), m_nextItemId(1), m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this)
{ 
	m_hourlyEvent.schedule(*this);
	m_path.append(L"save/"+name);
	m_dramaEngine = std::make_unique<DramaEngine>();
}
Simulation::Simulation(std::filesystem::path path) : Simulation(Json::parse(std::ifstream{path/"simulation.json"})) 
{
       	m_path = path;
	const Json& data = Json::parse(std::ifstream{m_path/"simulation.json"});
	for(const Json& areaId : data["areaIds"])
	{
		std::ifstream af(m_path/"area"/(std::to_string(areaId.get<AreaId>()) + ".json"));
		Json areaData = Json::parse(af);
		m_areas.emplace_back(areaData, m_deserializationMemo, *this);
	}
	m_dramaEngine = std::make_unique<DramaEngine>(data["drama"], m_deserializationMemo);
}
Simulation::Simulation(const Json& data) : m_deserializationMemo(*this), m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this) 
{
	m_name = data["name"].get<std::wstring>();
	m_step = data["step"].get<Step>();
	m_now = data["now"].get<DateTime>();
	m_nextItemId = data["nextItemId"].get<ItemId>();
	m_nextActorId = data["nextActorId"].get<ActorId>();
	m_nextAreaId = data["nextAreaId"].get<AreaId>();
	//if(data["world"])
		//m_world = std::make_unique<World>(data["world"], deserializationMemo);
	m_hasFactions.load(data["factions"], m_deserializationMemo);
	m_hourlyEvent.schedule(*this, data["hourEventStart"].get<Step>());
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
void Simulation::save()
{
	std::filesystem::create_directory(m_path);
	std::ofstream f(m_path/"simulation.json");
	f << toJson();
	std::filesystem::create_directory(m_path/"area");
	for(Area& area : m_areas)
	{
		std::ofstream af(m_path/"area"/(std::to_string(area.m_id) + ".json"));
		af << area.toJson();
	}
}
Faction& Simulation::createFaction(std::wstring name)
{
	return m_hasFactions.createFaction(name);
}
Area& Simulation::createArea(uint32_t x, uint32_t y, uint32_t z, bool createDrama)
{ 
	AreaId id = m_nextAreaId++;
	Area& output = loadArea(id, L"unnamed area " + std::to_wstring(id), x, y, z);
	if(createDrama)
		m_dramaEngine->createArcsForArea(output);
	return output;
}
Area& Simulation::loadArea(AreaId id, std::wstring name, uint32_t x, uint32_t y, uint32_t z)
{
	Area& area = m_areas.emplace_back(id, name, *this, x, y, z); 
	m_areasById[id] = &area;
	return area;
}
Actor& Simulation::createActor(ActorParamaters params)
{
	params.simulation = this;
	auto [iter, emplaced] = m_actors.emplace(
		params.getId(),
		params
	);
	assert(emplaced);
	return iter->second;
}
Actor& Simulation::createActor(const AnimalSpecies& species, Block& location, Percent percentGrown)
{
	return createActor(ActorParamaters{.species = species, .percentGrown = percentGrown, .location = &location});
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
	m_items.erase(item.m_id);
}
void Simulation::destroyArea(Area& area)
{
	m_dramaEngine->removeArcsForArea(area);
	for(Actor* actor : area.m_hasActors.getAll())
		actor->exit();
	m_areasById.erase(area.m_id);
	m_areas.remove(area);
}
void Simulation::destroyActor(Actor& actor)
{
	if(actor.m_location)
		actor.leaveArea();
	m_actors.erase(actor.m_id);
	//TODO: Destroy hibernation json file if any.
}
Simulation::~Simulation()
{
	for(Area& area : m_areas)
		area.clearReservations();
	for(auto& pair : m_actors)
		pair.second.m_canReserve.deleteAllWithoutCallback();
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
void Simulation::fastForwardUntillActorHasEquipment(Actor& actor, Item& item)
{
	std::function<bool()> predicate = [&](){ return actor.m_equipmentSet.contains(item); };
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
	output["name"] = m_name;
	//output["world"] = m_world;
	output["step"] = m_step;
	output["now"] = m_now;
	output["nextActorId"] = m_nextActorId;
	output["nextItemId"] = m_nextItemId;
	output["nextAreaId"] = m_nextAreaId;
	output["areaIds"] = Json::array();
	for(const Area& area : m_areas)
		output["areaIds"].push_back(area.m_id);
	output["factions"] = m_hasFactions.toJson();
	output["hourEventStart"] = m_hourlyEvent.getStartStep();
	output["drama"] = m_dramaEngine->toJson();
	return output;
}
Area& Simulation::loadAreaFromJson(const Json& data)
{
	DeserializationMemo deserializationMemo(*this);
	m_areas.emplace_back(data, deserializationMemo, *this);
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
	m_actors.try_emplace(id, data, deserializationMemo);
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
