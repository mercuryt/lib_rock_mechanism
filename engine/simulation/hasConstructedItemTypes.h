#include "../types.h"
#include "../itemType.h"
#include "../vectorContainers.h"
#include "../json.h"
class ConstructedShape;
class SimulationHasConstructedItemTypes
{
	SmallMap<ItemTypeId, ItemTypeParamaters> m_data;
	uint m_nextNameNumber = 0;
public:
	[[nodiscard]] ItemIndex createShip(Area& area, const BlockIndex& block, const Facing4& facing, const std::string& name, const FactionId& faction);
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasConstructedItemTypes, m_data, m_nextNameNumber);
};