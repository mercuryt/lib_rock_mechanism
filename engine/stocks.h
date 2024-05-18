#pragma once
#include <unordered_map>
#include <unordered_set>
#include <cassert>

struct ItemType;
struct MaterialType;
class Item;
struct Faction;

class AreaHasStocksForFaction final
{
	std::unordered_map<const ItemType*, std::unordered_map<const MaterialType*, std::unordered_set<Item*>>> m_data;
public:
	void record(Item& item);
	void unrecord(Item& item);
	const std::unordered_map<const ItemType*, std::unordered_map<const MaterialType*, std::unordered_set<Item*>>>& get() const { return m_data; }
};
class AreaHasStocks final
{
	std::unordered_map<Faction*, AreaHasStocksForFaction> m_data;
public:
	AreaHasStocksForFaction& at(Faction& faction) { assert(m_data.contains(&faction)); return m_data.at(&faction); }
	void addFaction(Faction& faction) { m_data.try_emplace(&faction); }
	void removeFaction(Faction& faction) { assert(m_data.contains(&faction)); m_data.erase(&faction); }
	[[nodiscard]] bool contains(Faction& faction) const { return m_data.contains(&faction); }
};
