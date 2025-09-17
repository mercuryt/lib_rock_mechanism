#include "installItem.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../deserializationMemo.h"
#include "../items/items.h"
#include "../reference.h"
#include "../path/terrainFacade.h"
#include "../definitions/moveType.h"

// Project.
InstallItemProject::InstallItemProject(Area& area, const ItemReference& i, const Point3D& l, const Facing4& facing, const FactionId& faction, const CuboidSet& occupied) :
	Project(faction, area, l, Quantity::create(1), nullptr, occupied), m_item(i), m_facing(facing) { }
void InstallItemProject::onComplete()
{
	Actors& actors = m_area.getActors();
	Items& items = m_area.getItems();
	ItemIndex item = m_item.getIndex(items.m_referenceData);
	const Point3D& location = m_location;
	const Facing4& facing = m_facing;
	auto workers = m_workers;
	m_area.m_hasInstallItemDesignations.getForFaction(m_faction).remove(m_area, item);
	// TODO: assert that space will not be overfull after placement.
	items.location_setStatic(item, location, facing);
	for(auto& [actor, projectWorker] : workers)
		actors.objective_complete(actor.getIndex(actors.m_referenceData), *projectWorker.objective);
}