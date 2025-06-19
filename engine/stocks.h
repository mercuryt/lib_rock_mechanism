#pragma once

#include "types.h"
#include "index.h"

#include <cassert>

struct ItemType;
struct MaterialType;
struct Faction;
class Area;

class AreaHasStocksForFaction final
{
	SmallMap<ItemTypeId, SmallMap<MaterialTypeId, SmallSet<ItemIndex>>> m_data;
public:
	void record(Area& area, ItemIndex item);
	void maybeRecord(Area& area, ItemIndex item);
	void unrecord(Area& area, ItemIndex item);
	const auto& get() const { return m_data; }
};
class AreaHasStocks final
{
	SmallMap<FactionId, AreaHasStocksForFaction> m_data;
public:
	AreaHasStocksForFaction& getForFaction(FactionId faction) { assert(m_data.contains(faction)); return m_data[faction]; }
	void addFaction(FactionId faction) { m_data.emplace(faction); }
	void removeFaction(FactionId faction) { assert(m_data.contains(faction)); m_data.erase(faction); }
	[[nodiscard]] bool contains(FactionId faction) const { return m_data.contains(faction); }
};
