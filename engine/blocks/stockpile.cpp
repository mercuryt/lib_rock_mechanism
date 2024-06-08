#include "blocks.h"
#include "../stockpile.h"
#include "item.h"
#include "shape.h"
#include "types.h"
void Blocks::stockpile_recordMembership(BlockIndex index, StockPile& stockPile)
{
	assert(!stockpile_contains(index, stockPile.getFaction()));
	auto result = m_stockPiles[index].try_emplace(&stockPile.getFaction(), stockPile, false);
	assert(result.second);
	if(stockpile_isAvalible(index, stockPile.getFaction()))
	{
		m_stockPiles.at(index).at(&stockPile.getFaction()).active = true;
		stockPile.incrementOpenBlocks();
	}
}
void Blocks::stockpile_recordNoLongerMember(BlockIndex index, StockPile& stockPile)
{
	assert(stockpile_contains(index, stockPile.getFaction()));
	if(stockpile_isAvalible(index, stockPile.getFaction()))
		stockPile.decrementOpenBlocks();
	if(m_stockPiles.at(index).size() == 1)
		m_stockPiles.erase(index);
	else
		m_stockPiles.at(index).erase(&stockPile.getFaction());
}
void Blocks::stockpile_updateActive(BlockIndex index)
{
	for(auto& [faction, blockHasStockPile] : m_stockPiles.at(index))
	{
		bool avalible = stockpile_isAvalible(index, *faction);
		if(avalible && !blockHasStockPile.active)
		{
			blockHasStockPile.active = true;
			blockHasStockPile.stockPile.incrementOpenBlocks();
		}
		else if(!avalible && blockHasStockPile.active)
		{
			blockHasStockPile.active = false;
			blockHasStockPile.stockPile.decrementOpenBlocks();
		}
	}
}
bool Blocks::stockpile_isAvalible(BlockIndex index, Faction& faction) const 
{ 
	assert(stockpile_contains(index, faction));
	if(item_empty(index))
		return true;
	if(item_getAll(index).size() > 1)
		return false;
	Item& item = *item_getAll(index).front();
	if(!item.isGeneric())
		return false;
	StockPile& stockPile = m_stockPiles.at(index).at(&faction).stockPile;
	if(!stockPile.accepts(item))
		return false;
	CollisionVolume volume = item.m_shape->getCollisionVolumeAtLocationBlock();
	return shape_getStaticVolume(index) + volume <= Config::maxBlockVolume;
}
bool Blocks::stockpile_contains(BlockIndex index, Faction& faction) const
{
	auto found = m_stockPiles.find(index);
	if(found == m_stockPiles.end())
		return false;
	return found->second.contains(&faction);
}
StockPile* Blocks::stockpile_getForFaction(BlockIndex index, Faction& faction)
{
	auto indexIter = m_stockPiles.find(index);
	if(indexIter == m_stockPiles.end())
		return nullptr;
	auto factionIter = indexIter->second.find(&faction);
	if(factionIter == m_stockPiles.at(index).end())
		return nullptr;
	return &factionIter->second.stockPile;
}
