#pragma once

/*
 * Represents a hit which a body can recieve.
 */

#include "materialType.h"
#include "woundType.h"

struct Hit
{
	uint32_t area;
	uint32_t force;
	uint32_t depth;
	const MaterialType& materialType;
	const WoundType woundType;
	Hit(uint32_t a, uint32_t f, const MaterialType& mt, const WoundType wt) : area(a), force(f), depth(0), materialType(mt), woundType(wt) {}
};
