#pragma once

#include "../numericTypes/types.h"
#include "../config.h"
#include <unordered_map>

class Items;
struct DeserializationMemo;
class Area;

struct ItemDataLocation
{
	Items* store;
	ItemIndex index;
};

class SimulationHasItems final
{
	ItemId m_nextId = ItemId::create(0);
	// TODO: Use boost unordered_map.
	std::unordered_map<ItemId, ItemDataLocation, ItemId::Hash> m_items;
public:
	SimulationHasItems() = default;
	SimulationHasItems(const Json& data, DeserializationMemo& deserializationMemo);
	void registerItem(const ItemId& id, Items& store, const ItemIndex& index);
	void removeItem(const ItemId& id);
	[[nodiscard]] ItemId getNextId() { return ++m_nextId; }
	[[nodiscard]] ItemIndex getIndexForId(const ItemId& id) const;
	[[nodiscard]] Area& getAreaForId(const ItemId& id) const;
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimulationHasItems, m_nextId);
};
