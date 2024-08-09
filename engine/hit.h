#pragma once

/*
 * Represents a hit which a body can recieve.
 */

#include "config.h"
#include "materialType.h"
#include "woundType.h"

// TODO: HitParamaters.
struct Hit
{
	uint32_t area;
	Force force;
	uint32_t depth;
	MaterialTypeId materialType;
	const WoundType woundType;
	Hit(uint32_t a, Force f, MaterialTypeId mt, const WoundType wt) : area(a), force(f), depth(0), materialType(mt), woundType(wt) { }
	Hit(Json data) : area(data["area"].get<uint32_t>()), force(data["force"].get<Force>()), depth(data["depth"].get<uint32_t>()), materialType(MaterialType::byName(data["materialType"].get<std::string>())), woundType(woundTypeByName(data["woundType"].get<std::string>())) { }
	Json toJson() const 
	{
		Json data;
		data["area"] = area;
		data["force"] = force;
		data["depth"] = depth;
		data["materialType"] = materialType;
		data["woundType"] = getWoundTypeName(woundType);
		return data;
	}
};
