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

#include <stack>
#include <vector>
#include <unordered_set>

template<class DerivedActor, class Block, class MoveType, class FluidType>
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
	ScheduledEvent* m_taskEvent;
	uint32_t m_taskDelayCount;

	BaseActor(Block& l, const Shape& s, const MoveType& mt) : 
		HasShape<Block>(&s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(&mt), m_taskDelayCount(0)
	{
		setLocation(l);
	}
	BaseActor(const Shape& s, const MoveType& mt) : 
		HasShape<Block>(&s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(&mt), m_taskDelayCount(0) { }
	// Check location for route. If found set as own route and then register moving with area.
	// Else register route request with area. Syncronus.
	void setDestination(Block& block)
	{
		assert(&block != HasShape<Block>::m_location);
		assert(block.anyoneCanEnterEver() && block.shapeAndMoveTypeCanEnterEver(*HasShape<Block>::m_shape, *m_moveType));
		m_destination = &block;
		HasShape<Block>::m_location->m_area->registerRouteRequest(static_cast<DerivedActor&>(*this));
	}
	void setLocation(Block& block)
	{
		assert(block != *HasShape<Block>::m_location);
		assert(block.anyoneCanEnterEver() && block.shapeAndMoveTypeCanEnterEver(*HasShape<Block>::m_shape, *m_moveType));
		assert(block.actorCanEnterCurrently(static_cast<DerivedActor&>(*this)));
		block.enter(static_cast<DerivedActor&>(*this));
	}
	// User provided code.
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	void taskComplete();
	void doVision(std::unordered_set<DerivedActor*>&& actors);
	bool canSee(const DerivedActor& actor) const;
	void exposedToFluid(const FluidType& fluidType);
};
template<class DerivedActor, class Block, class MoveType, class FluidType>
uint32_t BaseActor<DerivedActor, Block, MoveType, FluidType>::s_nextId = 1;
