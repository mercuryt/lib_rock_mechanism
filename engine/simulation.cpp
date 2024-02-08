#include "simulation.h"
#include "animalSpecies.h"
#include "area.h"
#include "config.h"
#include "definitions.h"
#include "deserializationMemo.h"
#include "haul.h"
#include "threadedTask.h"
#include "util.h"
//#include "worldforge/world.h"
#include <cstdint>
#include <filesystem>
#include <functional>
Simulation::Simulation(std::wstring name, DateTime n, Step s) :  m_nextAreaId(1), m_name(name), m_step(s), m_now(n), m_nextActorId(1), m_nextItemId(1), m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this)
{ 
	m_hourlyEvent.schedule(*this);
	m_path.append(L"save/"+name);
}
Simulation::Simulation(std::filesystem::path path) : m_path(path), m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this) 
{
	std::ifstream simulationFile(path/"simulation.json");
	const Json& data = Json::parse(simulationFile);
	m_name = data["name"].get<std::wstring>();
	m_step = data["step"].get<Step>();
	m_now = data["now"].get<DateTime>();
	m_nextItemId = data["nextItemId"].get<ItemId>();
	m_nextActorId = data["nextActorId"].get<ActorId>();
	m_nextAreaId = data["nextAreaId"].get<AreaId>();
	DeserializationMemo deserializationMemo(*this);
	//if(data["world"])
		//m_world = std::make_unique<World>(data["world"], deserializationMemo);
	for(const Json& areaId : data["areaIds"])
	{
		std::ifstream af(path/"area"/(std::to_string(areaId.get<AreaId>()) + ".json"));
		Json areaData = Json::parse(af);
		m_areas.emplace_back(areaData, deserializationMemo, *this);
	}
	m_hourlyEvent.schedule(*this, data["hourlyEventStart"].get<Step>());
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
Area& Simulation::createArea(uint32_t x, uint32_t y, uint32_t z)
{ 
	AreaId id = m_nextAreaId++;
	return loadArea(id, L"unnamed area " + std::to_wstring(id), x, y, z);
}
Area& Simulation::loadArea(AreaId id, std::wstring name, uint32_t x, uint32_t y, uint32_t z)
{
	Area& area = m_areas.emplace_back(id, name, *this, x, y, z); 
	m_areasById[id] = &area;
	return area;
}
Actor& Simulation::createActor(const AnimalSpecies& species, Percent percentGrown, DateTime birthDate)
{
	if(!birthDate)
	{
		// Create a plausible birth date.
		Step steps;
		if(percentGrown == 100)
			steps = m_random.getInRange(species.stepsTillFullyGrown, species.deathAgeSteps[1]);
		else
			steps = util::scaleByPercent(species.stepsTillFullyGrown, percentGrown);
		birthDate = DateTime::fromPastBySteps(steps);
	}
	Attributes attributes(species, percentGrown);
	std::wstring name(species.name.begin(), species.name.end());
	name.append(std::to_wstring(m_nextActorId));
	auto [iter, emplaced] = m_actors.try_emplace(
		m_nextActorId,
		*this,
		m_nextActorId,
		name,
		species,
		birthDate,
		percentGrown,
		nullptr,
		attributes
	);	
	assert(emplaced);
	++m_nextActorId;
	Actor& output = iter->second;
	return output;
}
Actor& Simulation::createActor(const AnimalSpecies& species, Block& block, Percent percentGrown, DateTime birthDate)
{
	Actor& output = createActor(species, percentGrown, birthDate);
	output.setLocation(block);
	block.m_area->m_hasActors.add(output);
	return output;
}
Actor& Simulation::createActorWithRandomAge(const AnimalSpecies& species, Block& block)
{
	Percent percentLifeTime = m_random.getInRange(0, 100);
	Step ageSteps = util::scaleByPercent(species.deathAgeSteps[1], percentLifeTime);
	Percent percentGrown = std::min(100, util::fractionToPercent(ageSteps, species.stepsTillFullyGrown));
	auto birthDate = DateTime::fromPastBySteps(ageSteps);
	return createActor(species, block, percentGrown, birthDate);
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
	for(Actor* actor : area.m_hasActors.getAll())
		actor->exit();
	m_areasById.erase(area.m_id);
	m_areas.remove(area);
}
void Simulation::destroyActor(Actor& actor)
{
	m_actors.erase(actor.m_id);
	//TODO: Destroy hibernation json file if any.
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
	return output;
}
Area& Simulation::loadAreaFromJson(const Json& data)
{
	//TODO: embed data in structure with factions to allow json pointer access.
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
