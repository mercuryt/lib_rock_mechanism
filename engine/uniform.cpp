#include "uniform.h"
#include "block.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "simulation.h"
#include "threadedTask.h"
#include "actor.h"
#include <chrono>
#include <cinttypes>
#include <math.h>
UniformElement::UniformElement(const ItemType& itemType, uint32_t quantity, const MaterialType* materialType,[[maybe_unused]] uint32_t qualityMin) :
	itemQuery(itemType, materialType), quantity(quantity) { }
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
// Equip single item.
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
// Equip uniform.
UniformThreadedTask::UniformThreadedTask(UniformObjective& objective) : ThreadedTask(objective.m_actor.getThreadedTaskEngine()), m_objective(objective), m_findsPath(objective.m_actor, false) { }
void UniformThreadedTask::readStep()
{
	std::function<bool(const Block&)> predicate = [&](const Block& block){
		return m_objective.blockContainsItem(block);
	};
	m_findsPath.pathToAdjacentToPredicate(predicate);
}
void UniformThreadedTask::writeStep()
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
void UniformThreadedTask::clearReferences() { m_objective.m_threadedTask.clearPointer(); }
// UniformObjective
UniformObjective::UniformObjective(Actor& actor) : Objective(actor, Config::equipPriority), m_threadedTask(actor.getThreadedTaskEngine()), 
	m_elementsCopy(actor.m_hasUniform.get().elements), m_item(nullptr) { assert(actor.m_hasUniform.exists()); }
UniformObjective::UniformObjective(const Json& data, DeserializationMemo& deserializationMemo) : Objective(data, deserializationMemo), m_threadedTask(deserializationMemo.m_simulation.m_threadedTaskEngine) 
{ 
	m_actor.m_hasUniform.recordObjective(*this);
}
Json UniformObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	if(m_threadedTask.exists())
		data["threadedTask"] = true;
	return data;
}
void UniformObjective::execute()
{
	if(m_item)
	{
		if(!m_actor.isAdjacentTo(*m_item))
			m_actor.m_canMove.setDestinationAdjacentTo(*m_item);
		else
		{
			if(m_actor.m_equipmentSet.canEquipCurrently(*m_item))
				equip(*m_item);
			else
			{
				reset();
				execute();
			}
		}
	}		
	else
	{
		std::function<bool(const Block&)> predicate = [&](const Block& block){ return blockContainsItem(block); };
		Block* adjacent = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
		if(adjacent)
			equip(*getItemAtBlock(*adjacent));
		else
			m_threadedTask.create(*this);
	}
}
void UniformObjective::cancel()
{
	m_actor.m_canReserve.clearAll();
	m_threadedTask.maybeCancel();
	m_actor.m_hasUniform.clearObjective(*this);
}
void UniformObjective::reset()
{
	m_actor.m_canReserve.clearAll();
	m_threadedTask.maybeCancel();
	m_item = nullptr;
}
Item* UniformObjective::getItemAtBlock(const Block& block)
{
	for(Item* item : block.m_hasItems.getAll())
		for(auto& element : m_elementsCopy)
			if(element.itemQuery(*item))
				return item;
	return nullptr;
}
void UniformObjective::select(Item& item) { m_item = &item; }
void UniformObjective::equip(Item& item)
{
	for(auto& element : m_elementsCopy)
		if(element.itemQuery(item))
		{
			if(item.getQuantity() >= element.quantity)
				std::erase(m_elementsCopy, element);
			else
				element.quantity -= item.getQuantity();
			break;
		}
	item.exit();
	m_actor.m_equipmentSet.addEquipment(item);
	if(m_elementsCopy.empty())
		m_actor.m_hasObjectives.objectiveComplete(*this);
	else
	{
		reset();
		execute();
	}
}
void ActorHasUniform::set(Uniform& uniform)
{
	if(m_objective)
		m_objective->cancel();
	m_uniform = &uniform;
	std::unique_ptr<Objective> objective = std::make_unique<UniformObjective>(m_actor);
	m_objective = static_cast<UniformObjective*>(objective.get());
	m_actor.m_hasObjectives.addTaskToStart(std::move(objective));
}
void ActorHasUniform::unset()
{
	if(m_objective)
		m_objective->cancel();
	m_uniform = nullptr;
}
void ActorHasUniform::recordObjective(UniformObjective& objective)
{
	assert(!m_objective);
	m_objective = &objective;
}
void ActorHasUniform::clearObjective(UniformObjective& objective)
{
	assert(m_objective);
	assert(*m_objective == objective);
	m_objective = nullptr;
}