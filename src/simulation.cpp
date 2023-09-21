#include "simulation.h"
#include "area.h"
#include "threadedTask.h"
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
	// Do threaded tasks write step first so other tasks created on this turn will not run write without having run read.
	m_threadedTaskEngine.writeStep();
	for(Area& area : m_areas)
		area.writeStep();
	// TODO: Why does putting this line before area write steps cause a segfault in FillQueue::validate?
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
	//name.append(std::to_wstring(m_nextActorId));
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
	std::list<Item>::iterator iterator = m_items.emplace(m_items.end(), id, itemType, materialType, name, quality, percentWear, cj);
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
	std::list<Item>::iterator iterator = m_items.emplace(m_items.end(), id, itemType, materialType, quantity, cj);
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
	m_threadedTaskEngine.m_tasks.clear();
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
