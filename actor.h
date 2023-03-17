/*
 * Represents and actor which has a shape, location, visual range, destination, and response to exposure to fluids.
 * To be inherited from in Actor declaration, not to be instanced directly.
 */
#pragma once

#include "moveType.h"
#include "routeRequest.h"
#include "shape.h"
#include "hasShape.h"
#include "hasScheduledEvents.h"

#include <stack>
#include <vector>
#include <unordered_set>

class Block;
class Actor;

class baseActor : public HasShape
{	
	static uint32_t s_nextId;
public:
	uint32_t m_id;
	std::vector<Block*> m_blocks;
	Block* m_destination;
	std::shared_ptr<std::vector<Block*>> m_route;
	std::vector<Block*>::const_iterator m_routeIter;
	const MoveType* m_moveType;
	ScheduledEvent* m_taskEvent;

	baseActor(Block* l, const Shape* s, const MoveType* mt);
	// Check location for route. If found set as own route and then register moving with area.
	// Else register route request with area.
	void setDestination(Block* block);
	// Record volume in all blocks of shape and set m_location.
	void setLocation(Block* block);
	// Remove all references in current area.
	void exitArea();
	// User provided code.
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	void taskComplete();
	void doVision(std::unordered_set<Actor*> actors);
	bool canSee(Actor* actor) const;
	void exposedToFluid(const FluidType* fluidType);
	~baseActor();
};
uint32_t baseActor::s_nextId = 1;
