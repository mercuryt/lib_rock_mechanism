#pragma once

#include "../types.h"
#include "../config.h"

class Items;
struct DeserializationMemo;

struct ItemDataLocation
{
	Items& store;
	ItemIndex index;
};

class SimulationHasItems final
{
	ItemId m_nextId = 0;
	std::unordered_map<ItemId, ItemDataLocation> m_items;
public:
	SimulationHasItems() = default;
	SimulationHasItems(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	[[nodiscard]] ItemId getNextId() { return ++m_nextId; }
	void registerItem(ItemId id, Items& store, ItemIndex index);
	void removeItem(ItemId id);
};
