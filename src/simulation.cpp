#include "simulation.h"
#include "area.h"
#include "threadedTask.h"
#include <functional>
Simulation::Simulation(DateTime n, Step s) :  m_step(s), m_now(n), m_eventSchedule(*this), m_hourlyEvent(m_eventSchedule), m_threadedTaskEngine(*this)
{ 
	m_nextActorId = 1;
	m_nextItemId = 1;
	m_hourlyEvent.schedule(*this);
}
void Simulation::doStep()
{
	m_threadedTaskEngine.readStep();
	for(Area& area : m_areas)
		area.readStep();
	m_pool.wait_for_tasks();
	for(Area& area : m_areas)
		area.writeStep();
	// Do threaded tasks write step before events so other tasks created on this turn will not run write without having run read.
	m_threadedTaskEngine.writeStep();
	// Do scheduled events last to avoid unexpected state changes in thereaded task data between read and write.
	m_eventSchedule.execute(m_step);
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
Actor& Simulation::createActor(const AnimalSpecies& species, Block& block, Percent percentGrown)
{
	Attributes attributes(species, percentGrown);
	std::wstring name(species.name.begin(), species.name.end());
	name.append(std::to_wstring(m_nextActorId));
	Actor& output = m_actors.emplace_back(
		*this,
		m_nextActorId,
		name,
		species,
		percentGrown,
		nullptr,
		attributes
	);	
	m_nextActorId++;
	output.setLocation(block);
	return output;
}
// Nongeneric
// No name or id.
Item& Simulation::createItem(const ItemType& itemType, const MaterialType& materialType, uint32_t quality, Percent percentWear, CraftJob* cj)
{
	return createItem(itemType, materialType, "", quality, percentWear, cj);
}
// Name but no id.
Item& Simulation::createItem(const ItemType& itemType, const MaterialType& materialType, std::string name, uint32_t quality, Percent percentWear, CraftJob* cj)
{
	const uint32_t id = ++ m_nextItemId;
	return createItem(id, itemType, materialType, name, quality, percentWear, cj);
}
// Name and id.
Item& Simulation::createItem(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, std::string name, uint32_t quality, Percent percentWear, CraftJob* cj)
{
	if(m_nextItemId <= id) m_nextItemId = id + 1;
	std::list<Item>::iterator iterator = m_items.emplace(m_items.end(), *this, id, itemType, materialType, name, quality, percentWear, cj);
	iterator->m_iterator = iterator;
	iterator->m_dataLocation = &m_items;
	return *iterator;
}
// Generic
// No id.
Item& Simulation::createItem(const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj)
{

	const uint32_t id = ++ m_nextItemId;
	return createItem(id, itemType, materialType, quantity, cj);
}
// Id.
Item& Simulation::createItem(const uint32_t id, const ItemType& itemType, const MaterialType& materialType, uint32_t quantity, CraftJob* cj)
{
	if(m_nextItemId <= id) m_nextItemId = id + 1;
	std::list<Item>::iterator iterator = m_items.emplace(m_items.end(), *this, id, itemType, materialType, quantity, cj);
	iterator->m_iterator = iterator;
	return *iterator;
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
void Simulation::fastForwardUntillPredicate(std::function<bool()> predicate, Step maxSteps)
{
	assert(!predicate());
	Step lastStep = m_step + maxSteps;
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
