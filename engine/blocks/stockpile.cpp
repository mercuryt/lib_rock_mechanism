#include "area/area.h"
#include "blocks.h"
#include "../stockpile.h"
#include "items/items.h"
#include "shape.h"
#include "types.h"
void Blocks::stockpile_recordMembership(const BlockIndex& index, StockPile& stockPile)
{
	assert(!stockpile_contains(index, stockPile.getFaction()));
	m_stockPiles.getOrCreate(index).emplace(stockPile.getFaction(), &stockPile, false);
	if(stockpile_isAvalible(index, stockPile.getFaction()))
	{
		m_stockPiles[index][stockPile.getFaction()].active = true;
		stockPile.incrementOpenBlocks();
		m_area.m_blockDesignations.getForFaction(stockPile.getFaction()).set(index, BlockDesignation::StockPileHaulTo);
	}
}
void Blocks::stockpile_recordNoLongerMember(const BlockIndex& index, StockPile& stockPile)
{
	assert(stockpile_contains(index, stockPile.getFaction()));
	if(stockpile_isAvalible(index, stockPile.getFaction()))
	{
		stockPile.decrementOpenBlocks();
		m_area.m_blockDesignations.getForFaction(stockPile.getFaction()).unset(index, BlockDesignation::StockPileHaulTo);
	}
	if(m_stockPiles[index].size() == 1)
		m_stockPiles.erase(index);
	else
		m_stockPiles[index].erase(stockPile.getFaction());
}
void Blocks::stockpile_updateActive(const BlockIndex& index)
{
	for(auto& [faction, blockHasStockPile] : m_stockPiles[index])
	{
		bool avalible = stockpile_isAvalible(index, faction);
		if(avalible && !blockHasStockPile.active)
		{
			blockHasStockPile.active = true;
			blockHasStockPile.stockPile->incrementOpenBlocks();
		}
		else if(!avalible && blockHasStockPile.active)
		{
			blockHasStockPile.active = false;
			blockHasStockPile.stockPile->decrementOpenBlocks();
		}
	}
}
bool Blocks::stockpile_isAvalible(const BlockIndex& index, const FactionId& faction) const
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
bool Blocks::stockpile_contains(const BlockIndex& index, const FactionId& faction) const
{
	auto found = m_stockPiles.find(index);
	if(found == m_stockPiles.end())
		return false;
	return found.second().contains(faction);
}
StockPile* Blocks::stockpile_getForFaction(const BlockIndex& index, const FactionId& faction)
{
	auto indexIter = m_stockPiles.find(index);
	if(indexIter == m_stockPiles.end())
		return nullptr;
	auto factionIter = indexIter.second().find(faction);
	if(factionIter == m_stockPiles[index].end())
		return nullptr;
	return factionIter->second.stockPile;
}
