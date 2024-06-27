#include "uniform.h"
#include "area.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "simulation.h"
#include "threadedTask.h"
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

SimulationHasUniformsForFaction& SimulationHasUniforms::at(Faction& faction) 
{ 
	if(!m_data.contains(&faction))
		registerFaction(faction);
	return m_data.at(&faction); 
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
			BlockIndex block = m_findsPath.getBlockWhichPassedPredicate();
			if(!m_objective.blockContainsItem(block))
				// Destination no longer suitable.
				m_objective.m_threadedTask.create(m_objective);
			else
			{
				ItemIndex item = m_objective.getItemAtBlock(block);
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
			ItemIndex item = m_objective.getItemAtBlock(m_findsPath.getBlockWhichPassedPredicate());
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
UniformObjective::UniformObjective(ActorIndex actor) : Objective(actor, Config::equipPriority), m_threadedTask(actor.getThreadedTaskEngine()), 
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
		std::function<bool(BlockIndex)> predicate = [&](BlockIndex block){ return blockContainsItem(block); };
		BlockIndex adjacent = m_actor.getBlockWhichIsAdjacentWithPredicate(predicate);
		if(adjacent != BLOCK_INDEX_MAX)
			equip(*getItemAtBlock(adjacent));
		else
			m_threadedTask.create(*this);
	}
}
void UniformObjective::cancel()
{
	m_actor.m_canReserve.deleteAllWithoutCallback();
	m_threadedTask.maybeCancel();
	m_actor.m_hasUniform.clearObjective(*this);
}
void UniformObjective::reset()
{
	m_actor.m_canReserve.deleteAllWithoutCallback();
	m_threadedTask.maybeCancel();
	m_item = nullptr;
}
ItemIndex UniformObjective::getItemAtBlock(BlockIndex block)
{
	for(ItemIndex item : m_actor.m_area->getBlocks().item_getAll(block))
		for(auto& element : m_elementsCopy)
			if(element.itemQuery.query(*item))
				return item;
	return nullptr;
}
void UniformObjective::select(ItemIndex item) { m_item = &item; }
void UniformObjective::equip(ItemIndex item)
{
	for(auto& element : m_elementsCopy)
		if(element.itemQuery.query(item))
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
// Equip uniform.
UniformThreadedTask::UniformThreadedTask(UniformObjective& objective) : ThreadedTask(objective.m_actor.getThreadedTaskEngine()), m_objective(objective), m_findsPath(objective.m_actor, false) { }
void UniformThreadedTask::readStep()
{
	std::function<bool(BlockIndex)> predicate = [&](BlockIndex block){
		return m_objective.blockContainsItem(block);
	};
	m_findsPath.pathToAdjacentToPredicate(predicate);
}
