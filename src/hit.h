#pragma once

/*
 * Represents a hit which a body can recieve.
 */

struct Hit
{
	uint32_t area;
	uint32_t force;
	uint32_t depth;
	Hit(uint32_t a, uint32_t f) : area(a), force(f) {}
};
