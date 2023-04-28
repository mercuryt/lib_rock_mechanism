#pragma once

#include "shape.h"

class HasShape
{
public:
	const Shape* m_shape;
	DerivedBlock* m_location;
	HasShape(const Shape* s) : m_shape(s), m_location(nullptr) {}
};
