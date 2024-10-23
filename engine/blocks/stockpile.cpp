#include "area.h"
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
	}
}
void Blocks::stockpile_recordNoLongerMember(const BlockIndex& index, StockPile& stockPile)
{
	assert(stockpile_contains(index, stockPile.getFaction()));
	if(stockpile_isAvalible(index, stockPile.getFaction()))
		stockPile.decrementOpenBlocks();
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
	if(item_empty(index))
		return true;
	if(item_getAll(index).size() > 1)
		return false;
	ItemIndex item = item_getAll(index).front();
	Items& items = m_area.getItems();
	if(!items.isGeneric(item))
		return false;
	StockPile& stockPile = *m_stockPiles[index][faction].stockPile;
	if(!stockPile.accepts(item))
		return false;
	CollisionVolume volume = Shape::getCollisionVolumeAtLocationBlock(items.getShape(item));
	return shape_getStaticVolume(index) + volume <= Config::maxBlockVolume;
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
