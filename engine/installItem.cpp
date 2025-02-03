#include "installItem.h"
#include "actors/actors.h"
#include "area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "reference.h"
#include "path/terrainFacade.h"

// Project.
InstallItemProject::InstallItemProject(Area& area, const ItemReference& i, const BlockIndex& l, const Facing4& facing, const FactionId& faction) :
	Project(faction, area, l, Quantity::create(1)), m_item(i), m_facing(facing) { }
void InstallItemProject::onComplete()
{
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	ItemIndex item = m_item.getIndex(items.m_referenceData);
	assert(items.isGeneric(item));
	// No need to worry about generic merge reference invalidation here: installed item cannot be generic.
	items.setLocationAndFacing(item, m_location, m_facing);
	auto workers = m_workers;
	m_area.m_hasInstallItemDesignations.getForFaction(m_faction).remove(m_area, item);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Area& area, const BlockIndex& block, const ItemIndex& item, const Facing4& facing, const FactionId& faction)
{
	assert(!m_designations.contains(block));
	m_designations.try_emplace(block, area, area.getItems().getReference(item), block, facing, faction);
}
void AreaHasInstallItemDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_designations)
			pair2.second.clearReservations();
}
void HasInstallItemDesignationsForFaction::remove(Area& area, const ItemIndex& item)
{
	assert(m_designations.contains(area.getItems().getLocation(item)));
	m_designations.erase(area.getItems().getLocation(item));
}
