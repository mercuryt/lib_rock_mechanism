#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "util.h"
#include "blockFeature.h"
#include <unordered_map>
#include <vector>

template<class ConstructObjective>
class ConstructThreadedTask : ThreadedTask
{
	ConstructObjective& m_constructObjective;
	std::vector<Block*> m_result;
	ConstructThreadedTask(ConstructObjective& ho) : m_constructObjective(ho) {}
	void readStep()
	{
		auto destinationCondition = [&](Block* block)
		{
			return m_constructObjective.canConstructAt(*block);
		}
		m_result = path::getForActorToPredicate(m_constructObjective.m_actor, destinationCondition)
	}
	void writeStep()
	{
		if(m_result.empty())
			m_constructObjective.m_actor.m_hasObjectives.cannotFulfillObjective();
		else
		{
			// Destination block has been reserved since result was found, get a new one.
			if(m_result.back()->reservable.isFullyReserved())
				m_constructObjective.m_constructThrededTask.create(m_constructObjective);
			m_constructObjective.m_actor.setPath(m_result);
			m_result.back()->reservable.reserveFor(m_constructObjective.m_actor.m_canReserve);
		}
	}
}
template<class Actor, class Plant>
class ConstructObjective : Objective
{
	Actor& m_actor;
	HasThreadedTask<ConstructThreadedTask> m_constructThreadedTask;
	Project* m_project;
	Construct(Actor& a) : m_actor(a), m_project(nullptr) { }
	void execute()
	{
		if(canConstructAt(*m_actor.m_location))
		{
			for(Block* adjacent : m_actor.m_location->getAdjacentWithEdgeAdjacent())
				if(!adjacent.m_reservable.isFullyReserved() && m_actor.m_location->m_area->m_constructDesignations.contains(*adjacent))
					m_actor.m_location->m_area->m_constructDesignations.at(*adjacent).addWorker(m_actor);
		}
		else
			m_constructThreadedTask.create(*this);
	}
	bool canConstructAt(Block& block) const
	{
		if(block->m_reservable.isFullyReserved())
			return false;
		for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
			if(m_actor.m_location->m_area->m_constructDesignations.contains(*adjacent))
				return true;
		return false;
	}
};
class ConstructObjectiveType : ObjectiveType
{
	ConstructObjectiveType() : m_name("construct") { }
	bool canBeAssigned(Actor& actor)
	{
		return !actor.m_location->m_area->m_constructDesignations.empty();
	}
	ConstructObjective makeFor(Actor& actor)
	{
		return ConstructObjective(actor);
	}
};
template<class Block, class BlockFeatureType, class Actor>
class ConstructDesignations
{
	struct ConstructProject : Project<Actor>
	{
		const BlockFeatureType* blockFeatureType;
		const MaterialType& materialType;
		std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const
		{
			return materialType.constructionType.consumed;
		}
		std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const
		{
			return materialType.constructionType.unconsumed;
		}
		std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t quantity>> getByproducts() const
		{
			return materialType.constructionType.byproducts;
		}
	public:
		// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
		ConstructDesignation(Block& b, const BlockFeatureType* bft, const MaterialType& mt) : project(b, Config::maxNumberOfWorkersForConstructProject), blockFeatureType(bft), materialType(mt) { }
		void onComplete()
		{
			assert(!m_location.isSolid());
			if(blockFeatureType == nullptr)
				m_location.setSolid(m_materialType);
			else
				m_location.m_hasBlockFeatures.construct(blockFeatureType, m_materialType);
		}
		// What would the total delay time be if we started from scratch now with current workers?
		uint32_t getDelay() const
		{
			uint32_t totalScore = 0;
			for(Actor* actor : m_workers)
				totalScore += getWorkerConstructScore(*actor);
			return Config::constructScoreCost / totalScore;
		}
		uint32_t getWorkerConstructScore(Actor& actor) const
		{
			return (actor.m_strength * Config::constructStrengthModifier) + (actor.m_skillSet.get(SkillType::Construct) * Config::constructSkillModifier);
		}
	};
	std::unordered_map<Block*, ConstructDesignation> m_data;
public:
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Block& block, const BlockFeatureType* blockFeatureType)
	{
		assert(!contains(&block));
		m_data.emplace(&block, block, blockFeatureType);
	}
	void remove(Block& block)
	{
		assert(contains(&block));
	       	m_data.remove(&block); 
	}
	bool contains(Block& block) const { return m_data.contains(&block); }
	const BlockFeatureType* at(Block& block) const { return m_data.at(&block).blockFeatureType; }
	bool empty() const { return m_data.empty(); }
};
