#include "hit.h"
Hit::Hit(int a, const Force& f, const MaterialTypeId& mt, const WoundType& wt) : area(a), force(f), depth(0), materialType(mt), woundType(wt) { }
Hit::Hit(const Json& data) : area(data["area"].get<int>()), force(data["force"].get<Force>()), depth(data["depth"].get<int>()), materialType(MaterialType::byName(data["materialType"].get<std::string>())), woundType(woundTypeByName(data["woundType"].get<std::string>())) { }
Json Hit::toJson() const
{
	Json data;
	data["area"] = area;
	data["force"] = force;
	data["depth"] = depth;
	data["materialType"] = materialType;
	data["woundType"] = getWoundTypeName(woundType);
	return data;
}
PsycologyWeight Hit::pain() const { return {(float)(depth * area)}; }