#pragma once

#include "shape.h"

class BLOCK;

class HasShape
{
public:
	const Shape* m_shape;
	BLOCK* m_location;
	HasShape(const Shape* s) : m_shape(s), m_location(nullptr) {}
};
