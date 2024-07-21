#include "installItem.h"
#include "actors/actors.h"
#include "area.h"
#include "deserializationMemo.h"
#include "items/items.h"
#include "reference.h"
#include "terrainFacade.h"

// Project.
InstallItemProject::InstallItemProject(Area& area, ItemReference i, BlockIndex l, Facing facing, FactionId faction) :
	Project(faction, area, l, 1), m_item(i), m_facing(facing) { }
void InstallItemProject::onComplete() { m_area.getItems().setLocationAndFacing(m_item.getIndex(), m_location, m_facing); }
// HasDesignations.
void HasInstallItemDesignationsForFaction::add(Area& area, BlockIndex block, ItemIndex item, Facing facing, FactionId faction)
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
void HasInstallItemDesignationsForFaction::remove(Area& area, ItemIndex item)
{
	assert(m_designations.contains(area.getItems().getLocation(item)));
	m_designations.erase(area.getItems().getLocation(item));
}
