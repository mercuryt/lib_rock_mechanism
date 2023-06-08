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
