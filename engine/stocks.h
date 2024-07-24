#pragma once

#include "types.h"
#include "index.h"

#include <unordered_map>
#include <cassert>

struct ItemType;
struct MaterialType;
struct Faction;
class Area;

class AreaHasStocksForFaction final
{
	std::unordered_map<const ItemType*, std::unordered_map<const MaterialType*, ItemIndices>> m_data;
public:
	void record(Area& area, ItemIndex item);
	void unrecord(Area& area, ItemIndex item);
	const std::unordered_map<const ItemType*, std::unordered_map<const MaterialType*, ItemIndices>>& get() const { return m_data; }
};
class AreaHasStocks final
{
	std::unordered_map<FactionId, AreaHasStocksForFaction> m_data;
public:
	AreaHasStocksForFaction& at(FactionId faction) { assert(m_data.contains(faction)); return m_data.at(faction); }
	void addFaction(FactionId faction) { m_data.try_emplace(faction); }
	void removeFaction(FactionId faction) { assert(m_data.contains(faction)); m_data.erase(faction); }
	[[nodiscard]] bool contains(FactionId faction) const { return m_data.contains(faction); }
};
