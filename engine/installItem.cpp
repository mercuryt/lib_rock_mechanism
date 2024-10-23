#include "installItem.h"
#include "actors/actors.h"
#include "area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "reference.h"
#include "terrainFacade.h"

// Project.
InstallItemProject::InstallItemProject(Area& area, const ItemReference& i, const BlockIndex& l, const Facing& facing, const FactionId& faction) :
	Project(faction, area, l, Quantity::create(1)), m_item(i), m_facing(facing) { }
void InstallItemProject::onComplete()
{
	m_area.getItems().setLocationAndFacing(m_item.getIndex(), m_location, m_facing);
	auto workers = m_workers;
	Actors& actors = m_area.getActors();
	m_area.m_hasInstallItemDesignations.getForFaction(m_faction).remove(m_area, m_item.getIndex());
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(), *projectWorker.objective);
}
// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Area& area, const BlockIndex& block, const ItemIndex& item, const Facing& facing, const FactionId& faction)
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
