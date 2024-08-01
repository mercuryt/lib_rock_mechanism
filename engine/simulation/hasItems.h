#pragma once

#include "../types.h"
#include "../config.h"

class Items;
struct DeserializationMemo;
class Area;

struct ItemDataLocation
{
	Items& store;
	ItemIndex index;
};

class SimulationHasItems final
{
	ItemId m_nextId = ItemId::create(0);
	ItemIdMap<ItemDataLocation> m_items;
public:
	SimulationHasItems() = default;
	SimulationHasItems(const Json& data, DeserializationMemo& deserializationMemo);
	void registerItem(ItemId id, Items& store, ItemIndex index);
	void removeItem(ItemId id);
	[[nodiscard]] ItemId getNextId() { return ++m_nextId; }
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] ItemIndex getIndexForId(ItemId id) const;
	[[nodiscard]] Area& getAreaForId(ItemId id) const;
};
