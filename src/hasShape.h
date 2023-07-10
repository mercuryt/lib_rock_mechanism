/*
 * A shared base class for Actor and Item.
 */
#pragma once

#include "shape.h"
#include "leadAndFollow.h"

#include <unordered_set>
#include <cassert>

class Block;

class HasShape
{
protected:
	HasShape(const Shape& s, uint8_t f = 0) : m_shape(&s), m_location(nullptr), m_facing(f), m_canLead(*this), m_canFollow(*this) {}
public:
	const Shape* m_shape;
	Block* m_location;
	uint8_t m_facing; 
	std::unordered_set<Block*> m_blocks;
	CanLead m_canLead;
	CanFollow m_canFollow;
	bool isAdjacentTo(HasShape& other) const;
	virtual bool isItem() const = 0;
	virtual bool isActor() const = 0;
};
