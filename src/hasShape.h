#pragma once

#include "shape.h"
#include "reservable.h"

class HasShape
{
public:
	const Shape* m_shape;
	Block* m_location;
	uint32_t m_quantity;
	uint8_t m_facing; 
	std::vector<Block*> m_blocks;
	Reservable m_reservable;
	HasShape(const Shape* s) : m_shape(s), m_location(nullptr) {}
	virtual bool isItem() const = 0;
	virtual bool isActor() const = 0;
};
