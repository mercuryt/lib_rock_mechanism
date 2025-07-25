#pragma once
#include "../numericTypes/types.h"
#include "../definitions/itemType.h"
#include "../dataStructures/smallSet.h"
#include "../json.h"
class ConstructedShape;
class SimulationHasConstructedItemTypes
{
	SmallMap<ItemTypeId, ItemTypeParamaters> m_data;
	uint m_nextNameNumber = 0;
public:
	// After deserialization of the simulation call 'load' to load stored item type paramaters into ItemType.
	void load();
	[[nodiscard]] ItemIndex createShip(Area& area, const Point3D& point, const Facing4& facing, const std::string& name, const FactionId& faction);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasConstructedItemTypes, m_data, m_nextNameNumber);
};