#pragma once

template<class DerivedBlock, class DerivedActor, class DerivedArea>
BaseActor<DerivedBlock, DerivedActor, DerivedArea>::BaseActor(DerivedBlock* l, const Shape* s, const MoveType* mt) : 
	HasShape<DerivedBlock>(s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(mt), m_taskDelayCount(0)
{
	setLocation(l);
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
BaseActor<DerivedBlock, DerivedActor, DerivedArea>::BaseActor(const Shape* s, const MoveType* mt) : 
	HasShape<DerivedBlock>(s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(mt), m_taskDelayCount(0) { }
// Check location for route. If found set as own route and then register moving with area.
// Else register route request with area. Syncronus.
template<class DerivedBlock, class DerivedActor, class DerivedArea>
void BaseActor<DerivedBlock, DerivedActor, DerivedArea>::setDestination(DerivedBlock& block)
{
	assert(&block != HasShape<DerivedBlock>::m_location);
	assert(block.anyoneCanEnterEver() && block.shapeAndMoveTypeCanEnterEver(HasShape<DerivedBlock>::m_shape, m_moveType));
	m_destination = &block;
	HasShape<DerivedBlock>::m_location->m_area->registerRouteRequest(static_cast<DerivedActor&>(*this));
}
// nullptr is a valid value for block.
template<class DerivedBlock, class DerivedActor, class DerivedArea>
void BaseActor<DerivedBlock, DerivedActor, DerivedArea>::setLocation(DerivedBlock* block)
{
	assert(block != HasShape<DerivedBlock>::m_location);
	assert(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(HasShape<DerivedBlock>::m_shape, m_moveType));
	assert(block->actorCanEnterCurrently(static_cast<DerivedActor&>(*this)));
	block->enter(static_cast<DerivedActor&>(*this));
}
template<class DerivedBlock, class DerivedActor, class DerivedArea>
BaseActor<DerivedBlock, DerivedActor, DerivedArea>::~BaseActor()
{
	if(HasShape<DerivedBlock>::m_location != nullptr)
		HasShape<DerivedBlock>::m_location->m_area->m_locationBuckets.erase(static_cast<DerivedActor&>(*this));
}
