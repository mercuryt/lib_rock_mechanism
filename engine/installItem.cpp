#include "installItem.h"
#include "actors/actors.h"
#include "area/area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "reference.h"
#include "path/terrainFacade.h"
#include "moveType.h"

// Project.
InstallItemProject::InstallItemProject(Area& area, const ItemReference& i, const BlockIndex& l, const Facing4& facing, const FactionId& faction, const BlockIndices& occupied) :
	Project(faction, area, l, Quantity::create(1), nullptr, occupied), m_item(i), m_facing(facing) { }
void InstallItemProject::onComplete()
{
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	ItemIndex item = m_item.getIndex(items.m_referenceData);
	const BlockIndex& location = m_location;
	const Facing4& facing = m_facing;
	auto workers = m_workers;
	m_area.m_hasInstallItemDesignations.getForFaction(m_faction).remove(m_area, item);
	// TODO: assert that blocks will not be overfull after placement.
	items.location_setStatic(item, location, facing);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}
// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Area& area, const BlockIndex& block, const ItemIndex& item, const Facing4& facing, const FactionId& faction)
{
	assert(!m_designations.contains(block));
	Items& items = area.getItems();
	const BlockIndices& occupied = Shape::getBlocksOccupiedAt(items.getCompoundShape(item), area.getBlocks(), block, facing);
	m_designations.emplace(block, area, items.getReference(item), block, facing, faction, occupied);
}
void AreaHasInstallItemDesignations::clearReservations()
{
	for(auto& pair : m_data)
		for(auto& pair2 : pair.second.m_designations)
			pair2.second->clearReservations();
}
void HasInstallItemDesignationsForFaction::remove(Area& area, const ItemIndex& item)
{
	assert(m_designations.contains(area.getItems().getLocation(item)));
	m_designations.erase(area.getItems().getLocation(item));
}
