#pragma once

baseActor::baseActor(Block* l, const Shape* s, const MoveType* mt) : 
	HasShape(s), m_id(s_nextId++), m_moveType(mt)
{
	setLocation(l);
}
// Check location for route. If found set as own route and then register moving with area.
// Else register route request with area. Syncronus.
void baseActor::setDestination(Block* block)
{
	assert(block != nullptr);
	assert(block != m_location);
	assert(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(m_shape, m_moveType));
	m_destination = block;
	m_location->m_area->registerRouteRequest(static_cast<Actor*>(this));
}
void baseActor::setLocation(Block* block)
{
	assert(block != nullptr);
	assert(block != m_location);
	assert(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(m_shape, m_moveType));
	assert(block->canEnterCurrently(static_cast<Actor*>(this)));
	block->enter(static_cast<Actor*>(this));
}
baseActor::~baseActor()
{
	if(m_location != nullptr)
		m_location->m_area->m_locationBuckets.erase(static_cast<Actor*>(this));
}
