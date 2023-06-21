/*
 * Represents and actor which has a shape, location, visual range, destination, and response to exposure to fluids.
 * To be inherited from in DerivedActor declaration, not to be instanced directly.
 */
#pragma once

#include "moveType.h"
#include "routeRequest.h"
#include "shape.h"
#include "hasShape.h"
#include "eventSchedule.h"
#include "visionRequest.h"
#include "queuedAction.h"
#include "objective.h"
#include "publishedEvents.h"

#include <stack>
#include <vector>
#include <unordered_set>
#include <queue>
#include <unordered_multimap>

template<class DerivedActor, class Block, class MoveType, class FluidType, class Faction>
class BaseActor : public HasShape<Block>
{	
	static uint32_t s_nextId;
public:
	uint32_t m_id;
	std::string m_name;
	std::vector<Block*> m_blocks;
	Block* m_destination;
	std::shared_ptr<std::vector<Block*>> m_route;
	std::vector<Block*>::const_iterator m_routeIter;
	const MoveType* m_moveType;
	uint32_t m_moveSpeed;
	uint32_t m_taskDelayCount;
	uint32_t m_mass;
	uint32_t m_visionRange;
	uint32_t m_maxAttackRange;
	uint32_t m_combatScore;
	bool m_alive;
	ScheduledEvent* m_actionEvent;
	std::deque<std::unique_ptr<QueuedAction>> m_actionQueue;
	std::priority_queue<std::unique_ptr<Objective>, std::vector<std::unique_ptr<Objective>, decltype(ObjectiveSort)> m_objectiveQueue;
	Faction* m_faction;
	EventPublisher m_eventPublisher;

	BaseActor(Block& l, const Shape& s, const MoveType& mt, uint32_t ms, uint32_t m, Faction* f) : 
		HasShape<Block>(&s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(&mt), m_moveSpeed(ms), m_taskDelayCount(0), m_mass(m), m_alive(true), m_actionEvent(nullptr), m_objectiveQueue(ObjectiveSort), m_faction(f)
	{
		setLocation(l);
		setCombatScore();
	}
	BaseActor(const Shape& s, const MoveType& mt, uint32_t ms, uint32_t m, Faction* f) : 
		HasShape<Block>(&s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(&mt), m_moveSpeed(ms), m_taskDelayCount(0),  m_mass(m), m_alive(true), m_faction(f)
	{
		setCombatScore();
	}
	DerivedActor& derived()
	{
		return static_cast<DerivedActor&>(*this);
	}
	// Check location for route. If found set as own route and then register moving with area.
	// Else register route request with area. Syncronus.
	void setDestination(Block& block)
	{
		assert(&block != HasShape<Block>::m_location);
		assert(block.anyoneCanEnterEver() && block.shapeAndMoveTypeCanEnterEver(getShape(), *m_moveType));
		m_destination = &block;
		HasShape<Block>::m_location->m_area->registerRouteRequest(derived());
	}
	void setLocation(Block& block)
	{
		assert(block != *HasShape<Block>::m_location);
		assert(block.anyoneCanEnterEver() && block.shapeAndMoveTypeCanEnterEver(getShape(), *m_moveType));
		assert(block.actorCanEnterCurrently(derived()));
		block.enter(derived());
	}
	Block& getLocation() const { return *HasShape<Block>::m_location; }
	Block& getShape() const { return *HasShape<Block>::m_shape; }
	void addObjective(std::unique_ptr<Objective>&& objective)
	{
		auto pointer = objective.get();
		m_objectiveQueue.insert(std::move(objective));
		if(m_objectiveQueue.top().get() == pointer)
		{
			m_actionQueue.clear();
			m_objectiveQueue.top().execute();
		}
	}
	void objectiveComplete()
	{
		assert(m_actionQueue.empty());
		assert(!m_objectiveQueue.empty());
		m_objectiveQueue.pop();
		if(m_objectiveQueue.empty())
			// Get another objective from task set.
			derived().onIdle();
		else
			m_objectiveQueue.top().execute();
	}
	void taskComplete()
	{
		if(m_actionQueue.front().isComplete())
			m_actionQueue.pop_front();
		if(m_actionQueue.empty())
			objectiveComplete();
		else
			m_actionQueue.top().execute();
	}
	bool isEnemy(Actor& actor) { return m_faction.m_enemies.contains(actor.m_faction); }
	bool isAlly(Actor& actor) { return m_faction.m_allies.contains(actor.m_faction); }
	// User provided code.
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	void onIdle;();
	void doVision(std::unordered_set<DerivedActor*>&& actors);
	bool canSee(const DerivedActor& actor) const;
	void exposedToFluid(const FluidType& fluidType);
};
template<class DerivedActor, class Block, class MoveType, class FluidType>
uint32_t BaseActor<DerivedActor, Block, MoveType, FluidType>::s_nextId = 1;
// To be used by block.
template<class Block, class Actor>
class HasActors
{
	Block& m_block;
	uint32_t m_volume;
	std::vector<Actor*> m_actors;
	std::unordered_map<const Shape*, std::unordered_map<const MoveType*, std::vector<std::pair<Block*, uint32_t>>>> m_moveCostsCache;
public:
	HasActors(Area& a) : m_area(a), m_volume(0) { }
	void enter(Actor& actor)
	{
		assert(actor.m_location != &m_block);
		assert(canAdd(actor));
		assert(std::ranges::find(m_actors, &actor) == m_actors.end());
		if(actor.m_locaton != nullptr)
			actor.m_locaton->m_hasActors.exit(actor);
		actor.m_location = &m_block;
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : item.itemType.shape.positions)
		{
			Block* block = m_block.offset(m_x, m_y, m_z);
			assert(block != nullptr);
			assert(!block->isSolid());
			assert(std::ranges::find(block->m_actors, &actor) == m_actors.end());
			block->m_hasActors.volume += v;
			block->m_hasActors.m_actors.push_back(&item);
		}
	}
	void exit(Actor& actor)
	{
		assert(actor.m_location == m_block);
		assert(canEnter(actor));
		assert(std::ranges::find(m_actors, &actor) != m_actors.end());
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : item.itemType.shape.positions)
		{
			Block* block = m_block.offset(m_x, m_y, m_z);
			assert(block != nullptr);
			assert(!block->isSolid());
			assert(std::ranges::find(block->m_actors, &actor) != m_actors.end());
			block->m_hasActors.volume -= v;
			std::erase(block->m_hasActors.m_actors, &actor);
		}
		actor.m_locaton = nullptr;
	}
	bool canEnterEver(Actor& actor) const
	{
		if(m_block.isSolid())
			return false;
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : actor.shape.positions)
		{
			Block* block = m_block.offset(m_x, m_y, m_z);
			if(block == nullptr)
				return false;
			if(block->isSolid())
				return false;
			if(!Config::moveTypeCanEnter(&block, actor.m_moveType))
				return false;
		}
		return true;
	}
	bool canEnterCurrently(Actor& actor) const
	{
		assert(canAddEver(actor));
		//TODO: rotation.
		for(auto& [m_x, m_y, m_z, v] : actor.shape.positions)
		{
			Block* block = m_block.offset(m_x, m_y, m_z);
			if(block->m_hasActors.m_volume + v > Config::maxBlockVolume)
				return false;
		}
		return true;
	}
	std::vector<Block*, uint32_t> getMoveCosts(const Shape& shape, const MoveType& moveType) const
	{
		std::vector<Block*, uint32_t> output;
		for(Block* block : m_block.m_adjacentsVector)
			if(Config::moveTypeCanEnterEverFromTo(moveType, m_block, block))
				output.emplace_back(block, Config::getMoveCostFromTo(moveType, m_block, block));
		return output;
	}
	void clearCache()
	{
		m_moveCostsCache.clear();
		for(Block* block : getAdjacentWithEdgeAndCornerAdjacent())
			block->m_hasActors.m_moveCostsCache.clear();
		m_block->m_area.invalidateAllCachedRoutes();
	}
};
