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
	int area;
	Force force;
	int depth;
	MaterialTypeId materialType;
	WoundType woundType;
	Hit(int a, const Force& f, const MaterialTypeId& mt, const WoundType& wt);
	Hit(const Json& data);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] PsycologyWeight pain() const;
};
