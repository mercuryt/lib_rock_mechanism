#pragma once
#include "../numericTypes/types.h"
#include "../definitions/itemType.h"
#include "../dataStructures/smallMap.h"
#include "../json.h"
class ConstructedShape;
struct CuboidSet;
class SimulationHasConstructedItemTypes
{
	SmallMap<ItemTypeId, ItemTypeParamaters> m_data;
	int m_nextNameNumber = 0;
public:
	// After deserialization of the simulation call 'load' to load stored item type paramaters into ItemType.
	void load();
	[[nodiscard]] ItemIndex createShip(Area& area, const Point3D& point, const Facing4& facing, const std::string& name, const FactionId& faction);
	[[nodiscard]] ItemIndex createPlatform(Area& area, const CuboidSet& cuboids, const Facing4& facing = Facing4::North, const std::string& name = "", const FactionId& faction = FactionId::null());
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasConstructedItemTypes, m_data, m_nextNameNumber);
};