#include "area/area.h"
#include "space.h"
#include "../area/stockpile.h"
#include "items/items.h"
#include "definitions/shape.h"
#include "numericTypes/types.h"
void Space::stockpile_recordMembership(const Point3D& point, StockPile& stockPile)
{
	m_stockPiles.getOrCreate(stockPile.getFaction()).insert(point, RTreeDataWrapper<StockPile*, nullptr>(&stockPile));
	if(stockpile_isAvalible(point, stockPile.getFaction()))
	{
		stockPile.incrementOpenPoints();
		m_area.m_spaceDesignations.getForFaction(stockPile.getFaction()).set(point, SpaceDesignation::StockPileHaulTo);
	}
}
template<typename ShapeT>
StockPile* Space::stockpile_getOneForFaction(const ShapeT& shape, const FactionId& faction)
{
	const auto found =  m_stockPiles.find(faction);
	if(found == m_stockPiles.end())
		return nullptr;
	return found->second.queryGetOne(shape).get();

}
template StockPile* Space::stockpile_getOneForFaction(const Point3D& shape, const FactionId& faction);
template StockPile* Space::stockpile_getOneForFaction(const Cuboid& shape, const FactionId& faction);
template<typename ShapeT>
const StockPile* Space::stockpile_getOneForFaction(const ShapeT& shape, const FactionId& faction) const
{
	return const_cast<Space*>(this)->stockpile_getOneForFaction(shape, faction);
}
template const StockPile* Space::stockpile_getOneForFaction(const Cuboid& shape, const FactionId& faction) const;
void Space::stockpile_recordNoLongerMember(const Point3D& point, StockPile& stockPile)
{
	assert(stockpile_contains(point, stockPile.getFaction()));
	m_stockPiles[stockPile.getFaction()].removeAll(point);
	const FactionId& faction = stockPile.getFaction();
	if(stockpile_isAvalible(point, stockPile.getFaction()))
	{
		stockPile.decrementOpenPoints();
		m_area.m_spaceDesignations.getForFaction(faction).unset(point, SpaceDesignation::StockPileHaulTo);
	}
	if(m_stockPiles[faction].leafCount() == 1)
		m_stockPiles.erase(faction);
	else
		m_stockPiles[faction].removeAll(point);
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
	StockPile& stockPile = *m_stockPiles[faction].queryGetOne(index).get();
	if(!stockPile.accepts(toCombineWith))
		return false;
	const ItemTypeId& itemType = items.getItemType(toCombineWith);
	const ShapeId& shape = ItemType::getShape(itemType);
	const Facing4& facing = items.getFacing(toCombineWith);
	return shape_staticCanEnterCurrentlyWithFacing(index, shape, facing, {});
}
bool Space::stockpile_contains(const Point3D& point, const FactionId& faction) const
{
	const auto found = m_stockPiles.find(faction);
	if(found == m_stockPiles.end())
		return false;
	return found->second.queryAny(point);
}