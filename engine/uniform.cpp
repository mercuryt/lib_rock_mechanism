#include "uniform.h"
#include "block.h"
#include "threadedTask.h"
#include "actor.h"
#include <chrono>
#include <math.h>
UniformElement::UniformElement(const ItemType& itemType, const MaterialType* materialType,[[maybe_unused]] uint32_t quality, uint32_t q) :
	itemQuery(itemType, materialType), quantity(q) { }
Uniform& SimulationHasUniformsForFaction::createUniform(std::wstring& name, std::vector<UniformElement>& elements)
{
	auto pair = m_data.try_emplace(name, name, elements);
	assert(pair.second);
	return pair.first->second;
}
void SimulationHasUniformsForFaction::destroyUniform(Uniform& uniform)
{
	m_data.erase(uniform.name);
}
std::unordered_map<std::wstring, Uniform>& SimulationHasUniformsForFaction::getAll(){ return m_data; }
EquipItemThreadedTask::EquipItemThreadedTask(EquipItemObjective& objective) : ThreadedTask(objective.m_actor.getThreadedTaskEngine()), m_objective(objective), m_findsPath(objective.m_actor, false) { }
void EquipItemThreadedTask::readStep()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block){
		return m_objective.blockContainsItem(block);
	};
	m_findsPath.pathToAdjacentToPredicate(predicate);
}
void EquipItemThreadedTask::writeStep()
{
	if(m_findsPath.found())
	{
		if(!m_findsPath.areAllBlocksAtDestinationReservable(m_objective.m_actor.getFaction()))
			// Cannot reserve location, try again.
			m_objective.m_threadedTask.create(m_objective);
		else
		{
			Block* block = m_findsPath.getBlockWhichPassedPredicate();
			if(!m_objective.blockContainsItem(*block))
				// Destination no longer suitable.
				m_objective.m_threadedTask.create(m_objective);
			else
			{
				Item* item = m_objective.getItemAtBlock(*block);
				if(!item)
					m_objective.m_threadedTask.create(m_objective);
				else
				{
					m_objective.select(*item);
					m_findsPath.reserveBlocksAtDestination(m_objective.m_actor.m_canReserve);
					m_objective.m_actor.m_canMove.setPath(m_findsPath.getPath());
				}
			}
		}
	}
	else 
	{
		if(m_findsPath.m_useCurrentLocation)
		{
			Item* item = m_objective.getItemAtBlock(*m_findsPath.getBlockWhichPassedPredicate());
			if(!item)
				m_objective.m_threadedTask.create(m_objective);
			else
			{
				m_objective.select(*item);
				m_objective.execute();
			}
		}
		else
			m_objective.m_actor.m_hasObjectives.cannotFulfillObjective(m_objective);
	}
}
void EquipItemThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
EquipItemObjective::EquipItemObjective(Actor& actor, ItemQuery& itemQuery) : Objective(actor, Config::equipPriority), m_itemQuery(itemQuery), m_threadedTask(actor.getThreadedTaskEngine()) { }
Json EquipItemObjective::toJson() const
{
	Json data = Objective::toJson();
	data["itemQuery"] = m_itemQuery.toJson();
	data["item"] = m_item;
	return data;
}
void EquipItemObjective::execute()
{
	if(m_item)
	{
		if(!m_actor.isAdjacentTo(*m_item))
			m_actor.m_canMove.setDestinationAdjacentTo(*m_item);
		else
		{
			if(m_actor.m_equipmentSet.canEquipCurrently(*m_item))
				m_actor.m_equipmentSet.addEquipment(*m_item);
			else
				m_actor.m_hasObjectives.cannotFulfillObjective(*this);
		}
	}		
	else
		m_threadedTask.create(*this);
}
void EquipItemObjective::cancel()
{
	m_actor.m_canReserve.clearAll();
	m_threadedTask.maybeCancel();
}
void EquipItemObjective::reset()
{
	cancel();
	m_item = nullptr;
}
Item* EquipItemObjective::getItemAtBlock(const Block& block)
{
	for(Item* item : block.m_hasItems.getAll())
		if(m_itemQuery(*item))
			return item;
	return nullptr;
}
void EquipItemObjective::select(Item& item) { m_item = &item; }
