#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "util.h"
#include <unordered_map>

template<class DigObjective>
class DigThreadedTask : ThreadedTask
{
	DigObjective& m_digObjective;
	DigThreadedTask(DigObjective& ho) : m_digObjective(ho) {}
	void readStep()
	{
		auto pathCondition = [&](Block* block)
		{
			return block->anyoneCanEnterEver() && block->canEnterEver(m_digObjective.m_actor);
		}
		auto destinationCondition = [&](Block* block)
		{
			m_digObjective.canDigAt(*block);
		}
		m_result = util::findWithPathCondition(pathCondition, destinationCondition, *m_drinkObjective.m_animal.m_location)
	}
	void writeStep()
	{
		if(m_result == nullptr)
			m_digObjective.m_actor.cannotFulfillObjective();
		else
		{
			m_digObjective.m_actor.setDestination(m_result);
			m_result.reservable.reserveFor(m_digObjective.m_actor.m_canReserve);
		}
			
	}
}
template<class Actor, class Plant>
class DigObjective : Objective
{
	Actor& m_actor;
	Dig(Actor& a) : m_actor(a) {}
	HasScheduledEvent<DigEvent> m_digEvent;
	HasThreadedTask<DigThreadedTask> m_digThrededTask;
	void execute()
	{
		if(canDigAt(*m_actor.m_location))
		{
			for(Block* adjacent : m_actor.m_location->getAdjacentWithEdgeAdjacent())
				if(!adjacent.m_reservable.isFullyReserved() && m_actor.m_location->m_area->m_digDesignations.contains(*adjacent))
					//TODO: apply dig skill.
					m_actor.m_location->m_area->m_digDesignations.at(*adjacent).addWorker(m_actor);
		}
		else
			m_digThrededTask.create(*this);
	}
	bool canDigAt(Block& block) const
	{
		if(block->m_reservable.isFullyReserved())
			return false;
		for(Block* adjacent : block.getAdjacentWithEdgeAdjacent())
			if(m_actor.m_location->m_area->m_digDesignations.contains(*adjacent))
					return true;
		return false;
	}
};
template<class Block, class BlockFeatureType, class Actor>
class DigDesignations
{
	struct DigProject : Project<Actor>
	{
		Block& block;
		const BlockFeatureType* blockFeatureType;
	public:
		DigDesignation(Block& b, const BlockFeatureType* bft) : block(b), blockFeatureType(bft) {}
		void complete()
		{
			if(blockFeatureType == nullptr)
				block.setNotSolid();
			else
				block.addHewnBlockFeature(blockFeatureType);
			//TODO: Generate spoil.
			dismissWorkers();
		}
		// What would the delay time be if we started from scratch now with current workers?
		uint32_t getDelay() const
		{
			uint32_t totalScore = 0;
			for(Actor* actor : m_workers)
				totalScore += getWorkerDigScore(*actor);
			return Config::digScoreCost / totalScore;
		}
		uint32_t getWorkerDigScore(Actor& actor)
		{
			return (actor.m_strength * Config::digStrengthModifier) + (actor.m_skillSet.get(SkillType::Dig) * Config::digSkillModifier);
		}
	};
	std::unordered_map<Block*, DigDesignation> m_data;
public:
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Block& block, const BlockFeatureType* blockFeatureType)
	{
		assert(!contains(&block));
		m_data.emplace(&block, block, blockFeatureType);
	}
	void remove(Block& block)
	{
		assert(contains(&block);
	       	m_data.remove(&block); 
	}
	bool contains(Block& block) const { return m_data.contains(&block); }
	const BlockFeatureType* at(Block& block) const { return m_data.at(&block).blockFeatureType; }
};
