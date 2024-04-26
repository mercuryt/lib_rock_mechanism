#pragma once

#include <unordered_set>

class Area;
class Item;

class AreaHasItems final
{
	Area& m_area;
	std::unordered_set<Item*> m_onSurface;
public:
	AreaHasItems(Area& a) : m_area(a) { }
	void setItemIsOnSurface(Item& item);
	void setItemIsNotOnSurface(Item& item);
	void onChangeAmbiantSurfaceTemperature();
	void remove(Item& item);
	friend class Item;
	// For testing.
	[[nodiscard, maybe_unused]] const std::unordered_set<Item*>& getOnSurfaceConst() const { return m_onSurface; }
};
