#pragma once

/*
 * Represents a hit which a body can recieve.
 */

#include "config.h"
#include "materialType.h"
#include "woundType.h"

struct Hit
{
	uint32_t area;
	uint32_t force;
	uint32_t depth;
	const MaterialType& materialType;
	const WoundType woundType;
	Hit(uint32_t a, uint32_t f, const MaterialType& mt, const WoundType wt) : area(a), force(f), depth(0), materialType(mt), woundType(wt) { }
	Hit(Json data) : area(data["area"].get<uint32_t>()), force(data["force"].get<uint32_t>()), depth(data["depth"].get<uint32_t>()), materialType(MaterialType::byName(data["materialType"].get<std::string>())), woundType(woundTypeByName(data["woundType"].get<std::string>())) { }
	Json toJson() const 
	{
		Json data;
		data["area"] = area;
		data["force"] = force;
		data["depth"] = depth;
		data["materialType"] = materialType.name;
		data["woundType"] = getWoundTypeName(woundType);
		return data;
	}
};
