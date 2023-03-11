#pragma once

#include "shape.h"

class Block;

class HasShape
{
public:
	const Shape* m_shape;
	Block* m_location;
	// Accessor methods allow derived classes to access without a class name qualifier?
	const Shape* getShape() { return m_shape; }
	Block* getLocation() { return m_location; }
	HasShape(const Shape* shape) : m_shape(shape) {}
};
