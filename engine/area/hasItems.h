#pragma once

#include "types.h"

#include <unordered_set>

class Area;

class AreaHasItems final
{
	Area& m_area;
	std::unordered_set<ItemIndex> m_onSurface;
public:
	AreaHasItems(Area& a) : m_area(a) { }
	void setItemIsOnSurface(ItemIndex item);
	void setItemIsNotOnSurface(ItemIndex item);
	void onChangeAmbiantSurfaceTemperature();
	void add(ItemIndex item);
	void remove(ItemIndex item);
	friend class Item;
	// For testing.
	[[nodiscard, maybe_unused]] const std::unordered_set<ItemIndex>& getOnSurfaceConst() const { return m_onSurface; }
};
