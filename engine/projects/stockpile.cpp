#include "stockpile.h"
#include "../area/area.h"
#include "../config.h"
#include "../deserializationMemo.h"
#include "../items/itemQuery.h"
#include "../items/items.h"
#include "../actors/actors.h"
#include "../blocks/blocks.h"
#include "../objectives/stockpile.h"
#include "../reference.h"
#include "../reservable.h"
#include "../simulation/simulation.h"
#include "../stocks.h"
#include "../path/terrainFacade.h"
#include "../numericTypes/types.h"
#include "../definitions/itemType.h"
#include "../util.h"
#include "../portables.hpp"
#include <cwchar>
#include <functional>
#include <memory>
#include <string>
StockPileProject::StockPileProject(const FactionId& faction, Area& area, const BlockIndex& block, const ItemIndex& item, const Quantity& quantity, const Quantity& maxWorkers) :
	Project(faction, area, block, maxWorkers),
	m_item(item, area.getItems().m_referenceData),
	m_quantity(quantity),
	m_itemType(area.getItems().getItemType(item)),
	m_materialType(area.getItems().getMaterialType(item)),
	m_stockpile(*area.getBlocks().stockpile_getForFaction(block, faction)) { }
StockPileProject::StockPileProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	Project(data, deserializationMemo, area),
	m_item(data["item"], area.getItems().m_referenceData),
	m_quantity(data["quantity"].get<Quantity>()),
	m_itemType(ItemType::byName(data["itemType"].get<std::string>())),
	m_materialType(MaterialType::byName(data["materialType"].get<std::string>())),
	m_stockpile(*deserializationMemo.m_stockpiles.at(data["stockpile"].get<uintptr_t>())) { }
Json StockPileProject::toJson() const
{
	Json data = Project::toJson();
	data["item"] = m_item;
	data["stockpile"] = reinterpret_cast<uintptr_t>(&m_stockpile);
	data["quantity"] = m_quantity;
	data["itemType"] = m_itemType;
	data["materialType"] = m_materialType;
	return data;
}
void StockPileProject::onComplete()
{
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	assert(m_deliveredItems.size() == 1 || m_itemAlreadyAtSite.size() == 1);
	ItemIndex index = m_item.getIndex(items.m_referenceData);
	auto workers = std::move(m_workers);
	FactionId faction = m_stockpile.m_faction;
	Area& area = m_area;
	auto& hasStockPiles = area.m_hasStockPiles.getForFaction(faction);
	BlockIndex location = m_location;
	hasStockPiles.maybeRemoveFromItemsWithDestinationByStockPile(m_stockpile, index);
	hasStockPiles.destroyProject(*this);
	items.location_clearStatic(index);
	// TODO: ensure shape can fit here, provide correct facing.
	index = items.location_set(index, location, Facing4::North);
	area.m_hasStocks.getForFaction(faction).maybeRecord(area, index);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
void StockPileProject::onReserve()
{
	// Project has reserved it's item and destination, as well as haul tool and / or beast of burden.
	// Clear from StockPile::m_projectNeedingMoreWorkers.
	if(m_stockpile.m_projectNeedingMoreWorkers == this)
		m_stockpile.m_projectNeedingMoreWorkers = nullptr;
}
void StockPileProject::onCancel()
{
	SmallSet<ActorIndex> workersAndCandidates = getWorkersAndCandidates();
	Actors& actors = m_area.getActors();
	Area& area = m_area;
	// This is destroyed here.
	m_area.m_hasStockPiles.getForFaction(m_faction).destroyProject(*this);
	for(ActorIndex actor : workersAndCandidates)
	{
		StockPileObjective& objective = actors.objective_getCurrent<StockPileObjective>(actor);
		// clear objective.m_project so reset does not try to cancel it.
		objective.m_project = nullptr;
		objective.reset(area, actor);
		actors.objective_canNotCompleteSubobjective(actor);
	}
}
void StockPileProject::onDelay()
{
	Items& items = m_area.getItems();
	m_area.getItems().stockpile_maybeUnsetAndScheduleReset(m_item.getIndex(items.m_referenceData), m_faction, Config::stepsToDisableStockPile);
	cancel();
}
void StockPileProject::onPickUpRequired(const ActorOrItemIndex& required)
{
	ItemReference ref = m_area.getItems().getReference(required.getItem());
	updateRequiredGenericReference(ref);
}
void StockPileProject::updateRequiredGenericReference(const ItemReference& newRef)
{
	if(newRef != m_item)
	{
		m_area.m_hasStockPiles.getForFaction(m_faction).updateItemReferenceForProject(*this, newRef);
		m_item = newRef;
	}
}
bool StockPileProject::canAddWorker(const ActorIndex& actor) const
{
	if(!Project::canAddWorker(actor))
		return false;
	return m_haulSubprojects.empty();
}
std::vector<std::pair<ItemQuery, Quantity>> StockPileProject::getConsumed() const { return {}; }
std::vector<std::pair<ItemQuery, Quantity>> StockPileProject::getUnconsumed() const { return {{ItemQuery::create(m_item), m_quantity}}; }
std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> StockPileProject::getByproducts() const {return {}; }
std::vector<ActorReference> StockPileProject::getActors() const { return {}; }