#pragma once

baseActor::baseActor() : m_id(s_nextId++) {}
// Check location for route. If found set as own route and then register moving with area.
// Else register route request with area. Syncronus.
void baseActor::setDestination(Block* block)
{
	m_destination = block;
	m_location->m_area->registerRouteRequest(static_cast<Actor*>(this));
}
