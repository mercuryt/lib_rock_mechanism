#pragma once

/*
 * Represents a hit which a body can recieve.
 */

#include "config/config.h"
#include "definitions/materialType.h"
#include "definitions/woundType.h"

// TODO: HitParamaters.
struct Hit
{
	int32_t area;
	Force force;
	int32_t depth;
	MaterialTypeId materialType;
	WoundType woundType;
	Hit(int32_t a, const Force& f, const MaterialTypeId& mt, const WoundType& wt);
	Hit(const Json& data);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] PsycologyWeight pain() const;
};
