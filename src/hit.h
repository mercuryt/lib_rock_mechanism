#pragma once

/*
 * Represents a hit which a body can recieve.
 */

#include "materialType.h"

struct Hit
{
	uint32_t area;
	uint32_t force;
	uint32_t depth;
	const MaterialType& materialType;
	Hit(uint32_t a, uint32_t f, const MaterialType& mt) : area(a), force(f), materialType(mt) {}
};
