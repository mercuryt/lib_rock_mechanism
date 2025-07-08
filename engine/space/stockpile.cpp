#include "area/area.h"
#include "space.h"
#include "../area/stockpile.h"
#include "items/items.h"
#include "definitions/shape.h"
#include "numericTypes/types.h"
void Space::stockpile_recordMembership(const Point3D& index, StockPile& stockPile)
{
	assert(!stockpile_contains(index, stockPile.getFaction()));
	m_stockPiles.getOrCreate(index).emplace(stockPile.getFaction(), &stockPile, false);
	if(stockpile_isAvalible(index, stockPile.getFaction()))
	{
		m_stockPiles[index][stockPile.getFaction()].active = true;
		stockPile.incrementOpenPoints();
		m_area.m_spaceDesignations.getForFaction(stockPile.getFaction()).set(index, SpaceDesignation::StockPileHaulTo);
	}
}
void Space::stockpile_recordNoLongerMember(const Point3D& index, StockPile& stockPile)
{
	assert(stockpile_contains(index, stockPile.getFaction()));
	if(stockpile_isAvalible(index, stockPile.getFaction()))
	{
		stockPile.decrementOpenPoints();
		m_area.m_spaceDesignations.getForFaction(stockPile.getFaction()).unset(index, SpaceDesignation::StockPileHaulTo);
	}
	if(m_stockPiles[index].size() == 1)
		m_stockPiles.erase(index);
	else
		m_stockPiles[index].erase(stockPile.getFaction());
}
void Space::stockpile_updateActive(const Point3D& index)
{
	for(auto& [faction, pointHasStockPile] : m_stockPiles[index])
	{
		bool avalible = stockpile_isAvalible(index, faction);
		if(avalible && !pointHasStockPile.active)
		{
			pointHasStockPile.active = true;
			pointHasStockPile.stockPile->incrementOpenPoints();
		}
		else if(!avalible && pointHasStockPile.active)
		{
			pointHasStockPile.active = false;
			pointHasStockPile.stockPile->decrementOpenPoints();
		}
	}
}
bool Space::stockpile_isAvalible(const Point3D& index, const FactionId& faction) const
{
	assert(stockpile_contains(index, faction));
	const auto& allItems = item_getAll(index);
	if(allItems.empty())
		return true;
	if(allItems.size() > 1)
		return false;
	ItemIndex toCombineWith = allItems.front();
	Items& items = m_area.getItems();
	if(!items.isGeneric(toCombineWith))
		return false;
	StockPile& stockPile = *m_stockPiles[index][faction].stockPile;
	if(!stockPile.accepts(toCombineWith))
		return false;
	const ItemTypeId& itemType = items.getItemType(toCombineWith);
	const ShapeId& shape = ItemType::getShape(itemType);
	const Facing4& facing = items.getFacing(toCombineWith);
	return shape_staticCanEnterCurrentlyWithFacing(index, shape, facing, {});
}
bool Space::stockpile_contains(const Point3D& index, const FactionId& faction) const
{
	auto found = m_stockPiles.find(index);
	if(found == m_stockPiles.end())
		return false;
	return found.second().contains(faction);
}
StockPile* Space::stockpile_getForFaction(const Point3D& index, const FactionId& faction)
{
	auto indexIter = m_stockPiles.find(index);
	if(indexIter == m_stockPiles.end())
		return nullptr;
	auto factionIter = indexIter.second().find(faction);
	if(factionIter == m_stockPiles[index].end())
		return nullptr;
	return factionIter->second.stockPile;
}
