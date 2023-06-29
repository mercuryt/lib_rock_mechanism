#pragma once

#include "objective.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "util.h"
#include "blockFeature.h"

#include <unordered_map>
#include <vector>

class DigThreadedTask : ThreadedTask
{
	DigObjective& m_digObjective;
	std::vector<Block*> m_result;
	DigThreadedTask(DigObjective& ho) : m_digObjective(ho) {}
	void readStep()
	{
		auto destinationCondition = [&](Block* block)
		{
			return m_digObjective.canDigAt(*block);
		}
		m_result = path::getForActorToPredicate(m_digObjective.m_actor, destinationCondition)
	}
	void writeStep()
	{
		if(m_result.empty())
			m_digObjective.m_actor.m_hasObjectives.cannotFulfillObjective();
		else
		{
			// Destination block has been reserved since result was found, get a new one.
			if(m_result.back()->reservable.isFullyReserved())
			{
				m_digObjective.m_digThrededTask.create(m_digObjective);
				return;
			}
			m_digObjective.m_actor.setPath(m_result);
			m_result.back()->reservable.reserveFor(m_digObjective.m_actor.m_canReserve);
		}
	}
};
class DigObjective : Objective
{
	Actor& m_actor;
	HasThreadedTask<DigThreadedTask> m_digThrededTask;
	Project* m_project;
	Dig(Actor& a) : m_actor(a), m_project(nullptr) { }
	void execute()
	{
		if(m_project != nullptr)
		{
			m_project.commandWorker(m_actor);
			return;
		}
		if(canDigAt(*m_actor.m_location))
		{
			for(Block* adjacent : m_actor.m_location->getAdjacentWithEdgeAdjacent())
				if(!adjacent.m_reservable.isFullyReserved() && m_actor.m_location->m_area->m_digDesignations.contains(*adjacent))
				{
					m_project = m_actor.m_location->m_area->m_digDesignations.at(*adjacent);
					m_project.addWorker(m_actor);
				}
		}
		else
			m_digThrededTask.create(*this);
	}
	bool canDigAt(Block& block) const
	{
		if(block->m_reservable.isFullyReserved())
			return false;
		for(Block* adjacent : block.getAdjacentWithEdgeAndCornerAdjacent())
			if(adjacent.m_hasDesignations.contains(*m_actor.m_player, BlockDesignation::Dig))
				return true;
		return false;
	}
};
class DigObjectiveType : ObjectiveType
{
	DigObjectiveType() : m_name("dig") { }
	bool canBeAssigned(Actor& actor)
	{
		return actor.m_location->m_area->m_digDesignations.areThereAnyForPlayer(m_actor.getPlayer());
	}
	std::unique_ptr<Objective> makeFor(Actor& actor)
	{
		return std::make_unique<DigObjective>(actor);
	}
};
class DigProject : Project
{
	const BlockFeatureType* blockFeatureType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const
       	{
	       	return {{ItemQuery::canDig, 1}}; 
	}
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t quantity>> getByproducts() const
	{
		std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> output;
		for(SpoilData& spoilData : m_location.getSolidMaterial().spoilData)
		{
			if(!randomUtil::chance(spoilData.chance))
				continue;
			uint32_t quantity = randomUtil::getInRange(spoilData.min, spoilData.max);
			m_location.m_hasItems.add(spoilData.itemType, spoilData.materialType, quantity);
		}
		return output;
	}
	static uint32_t getWorkerDigScore(Actor& actor) const
	{
		return (actor.m_strength * Config::digStrengthModifier) + (actor.m_skillSet.get(SkillType::Dig) * Config::digSkillModifier);
	}
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	DigProject(Block& b, const BlockFeatureType* bft) : project(b, Config::maxNumberOfWorkersForDigProject), blockFeatureType(bft) { }
	void onComplete()
	{
		const MaterialType& materialType = m_location.getSolidMaterial();
		if(blockFeatureType == nullptr)
			m_location.setNotSolid();
		else
			m_location.m_hasBlockFeatures.hue(blockFeatureType);
		// Remove designations for other players as well as owning player.
		m_block.m_location->m_area->m_hasDigDesignations.clearAll(m_block);

	}
	// What would the total delay time be if we started from scratch now with current workers?
	uint32_t getDelay() const
	{
		uint32_t totalScore = 0;
		for(Actor* actor : m_workers)
			totalScore += getWorkerDigScore(*actor);
		return Config::digScoreCost / totalScore;
	}
};
class HasDigDesignationsForPlayer
{
	Player& m_player;
	std::unordered_map<Block*, DigProject> m_data;
public:
	HasDigDesignationsForPlayer(Player& p) : m_player(p) { }
	void designate(Block& block, const BlockFeatureType* blockFeatureType)
	{
		assert(!contains(&block));
		block.m_hasDesignations.insert(m_player, BlockDesignation::Dig);
		m_data.emplace(&block, block, blockFeatureType);
	}
	void remove(Block& block)
	{
		assert(contains(&block));
		block.m_hasDesignations.remove(m_player, BlockDesignation::Dig);
		m_data.remove(&block); 
	}
	void removeIfExists(Block& block)
	{
		if(m_data.contains(&block))
			remove(block);
	}
	const BlockFeatureType* at(Block& block) const { return m_data.at(&block).blockFeatureType; }
	bool empty() const { return m_data.empty(); }
};
// To be used by Area.
class HasDigDesignations
{
	std::unordered_map<Player*, HasDigDesignationsForPlayer> m_data;
public:
	void addPlayer(Player& player)
	{
		assert(!m_data.contains(&player));
		m_data[&player](player);
	}
	void removePlayer(Player& player)
	{
		assert(m_data.contains(player));
		m_data.remove(&player);
	}
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(Player& player, Block& block, const BlockFeatureType* blockFeatureType)
	{
		m_data.at(player).designate(block, blockFeatureType);
	}
	void remove(Player& player, Block& block)
	{
		assert(m_data.contains(player));
		m_data.at(player).remove(block);
	}
	void clearAll(Block& block)
	{
		for(auto& pair : m_data)
			pair.second.removeIfExists(block);
	}
	void areThereAnyForPlayer(Player& player) const
	{
		return !m_data.at(player).empty();
	}
};
