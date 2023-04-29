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

template<class DerivedBlock, class DerivedActor, class DerivedArea>
class BaseActor : public HasShape
{	
	static uint32_t s_nextId;
public:
	uint32_t m_id;
	std::string m_name;
	std::vector<DerivedBlock*> m_blocks;
	DerivedBlock* m_destination;
	std::shared_ptr<std::vector<DerivedBlock*>> m_route;
	std::vector<DerivedBlock*>::const_iterator m_routeIter;
	const MoveType* m_moveType;
	ScheduledEvent* m_taskEvent;
	uint32_t m_taskDelayCount;

	BaseActor(DerivedBlock* l, const Shape* s, const MoveType* mt);
	BaseActor(const Shape* s, const MoveType* mt);
	// Set and then register route request with area.
	void setDestination(DerivedBlock& block);
	// Record volume in all blocks of shape and set m_location.
	// Can accept nullptr.
	void setLocation(DerivedBlock* block);
	// Remove all references in current area.
	void exitArea();
	// User provided code.
	uint32_t getSpeed() const;
	uint32_t getVisionRange() const;
	void taskComplete();
	void doVision(std::unordered_set<DerivedActor*>&& actors);
	bool canSee(const DerivedActor& actor) const;
	void exposedToFluid(const FluidType* fluidType);
	~BaseActor();
};
template<class DerivedBlock, class DerivedActor, class DerivedArea>
uint32_t BaseActor<DerivedBlock, DerivedActor, DerivedArea>::s_nextId = 1;
