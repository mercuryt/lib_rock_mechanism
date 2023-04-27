#pragma once

baseActor::baseActor(BLOCK* l, const Shape* s, const MoveType* mt) : 
	HasShape(s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(mt), m_taskDelayCount(0)
{
	setLocation(l);
}
baseActor::baseActor(const Shape* s, const MoveType* mt) : 
	HasShape(s), m_id(s_nextId++), m_name("actor#" + std::to_string(m_id)), m_moveType(mt), m_taskDelayCount(0) { }
// Check location for route. If found set as own route and then register moving with area.
// Else register route request with area. Syncronus.
void baseActor::setDestination(BLOCK& block)
{
	assert(&block != m_location);
	assert(block.anyoneCanEnterEver() && block.shapeAndMoveTypeCanEnterEver(m_shape, m_moveType));
	m_destination = &block;
	m_location->m_area->registerRouteRequest(static_cast<ACTOR&>(*this));
}
// nullptr is a valid value for block.
void baseActor::setLocation(BLOCK* block)
{
	assert(block != m_location);
	assert(block->anyoneCanEnterEver() && block->shapeAndMoveTypeCanEnterEver(m_shape, m_moveType));
	assert(block->actorCanEnterCurrently(static_cast<ACTOR&>(*this)));
	block->enter(static_cast<ACTOR&>(*this));
}
Cuboid getZLevel(uint32_t z) const
{
	return Cuboid(m_blocks[m_sizeX][m_sizeY][z], m_blocks[0][0][z]);
}
baseActor::~baseActor()
{
	if(m_location != nullptr)
		m_location->m_area->m_locationBuckets.erase(static_cast<ACTOR&>(*this));
}
