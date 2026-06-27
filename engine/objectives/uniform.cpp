#include "uniform.h"
#include "../actors/actors.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../items/items.h"
#include "../path/longRange.hpp"
#include "../numericTypes/types.h"
// Equip uniform.
UniformPathRequest::UniformPathRequest(Area& area, UniformObjective& objective, const ActorIndex actorIndex) :
	m_objective(objective)
{
	Actors& actors = area.getActors();
	start = actors.getLocation(actorIndex);
	maxRange = Config::maxRangeToSearchForDigDesignations;
	actor = actors.getReference(actorIndex);
	shape = actors.getShape(actorIndex);
	faction = actors.getFaction(actorIndex);
	moveType = actors.getMoveType(actorIndex);
	facing = actors.getFacing(actorIndex);
	detour = m_objective.m_detour;
	reserveDestination = true;
}
UniformPathRequest::UniformPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo) :
	PathRequest(data, area),
	m_objective(static_cast<UniformObjective&>(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())))
{ }
PathResult UniformPathRequest::readStep(Area& area, const AreaHasPathsForMoveType& hasPaths)
{
	auto shortRangeCondition = [&area, this](const Cuboid cuboid) -> Point3D
	{
		return m_objective.getLocationOfItemInCuboid(area, cuboid);
	};
	auto longRangeCondition = [&shortRangeCondition](const Cuboid cuboid) -> bool
	{
		return shortRangeCondition(cuboid) != Point3D::null();
	};
	return longRangePath::getPathAdjacent(hasPaths.m_enterable, longRangeCondition, shortRangeCondition, toParamaters(area));
}
void UniformPathRequest::writeStep(Area& area, bool useCurrentLocation)
{
	Actors& actors = area.getActors();
	ActorIndex actorIndex = actor.getIndex(actors.m_referenceData);
	if(!path.empty())
	{
		if(!actors.move_tryToReserveProposedDestination(actorIndex, path))
		{
			// Cannot reserve location, try again.
			m_objective.reset(area, actorIndex);
			m_objective.execute(area, actorIndex);
		}
		else
		{
			ItemIndex item = m_objective.getItemAtLocation(area, {target, target});
			if(item.empty())
			{
				// Destination no longer suitable.
				m_objective.reset(area, actorIndex);
				m_objective.execute(area, actorIndex);
			}
			else
			{
				m_objective.select(area, item);
				actors.move_setPath(actorIndex, path);
			}
		}
	}
	else
	{
		if(useCurrentLocation)
		{
			ItemIndex item = m_objective.getItemAtLocation(area, {target, target});
			if(item.empty())
			{
				m_objective.reset(area, actorIndex);
				m_objective.execute(area, actorIndex);
			}
			else
			{
				m_objective.select(area, item);
				m_objective.execute(area, actorIndex);
			}
		}
		else
			actors.objective_canNotCompleteObjective(actorIndex, m_objective);
	}
}
Json UniformPathRequest::toJson() const
{
	Json output = PathRequest::toJson();
	output["objective"] = &m_objective;
	output["type"] = "uniform";
	return output;
}
// UniformObjective
UniformObjective::UniformObjective(Area& area, const ActorIndex actor) :
	Objective(Config::equipPriority), m_elementsCopy(area.getActors().uniform_get(actor).elements)
{
	assert(area.getActors().uniform_exists(actor));
}
UniformObjective::UniformObjective(const Json& data, Area& area, const ActorIndex actor, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo)
{
	area.getActors().m_hasUniform[actor]->recordObjective(*this);
}
Json UniformObjective::toJson() const
{
	Json data = Objective::toJson();
	data["item"] = m_item;
	return data;
}
void UniformObjective::execute(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	if(m_item.exists())
	{
		ItemIndex item = m_item.getIndex(items.m_referenceData);
		if(!actors.isIntersectingOrAdjacentTo(actor, item))
			actors.move_setDestinationAdjacentToItem(actor, item);
		else
		{
			if(actors.equipment_canEquipCurrently(actor, item))
				equip(area, item, actor);
			else
			{
				reset(area, actor);
				execute(area, actor);
			}
		}
	}
	else
	{
		CuboidSet adjacent = actors.getAdjacentCuboids(actor);
		// Collect any which are adjacent at location.
		for(const Cuboid cuboid : adjacent)
		{
			const ItemIndex item = getItemAtLocation(area, cuboid);
			if(item.exists())
			{
				equip(area, item, actor);
				return;
			}
		}
		// Try to path to next location.
		actors.move_pathRequestRecord(actor, std::make_unique<UniformPathRequest>(area, *this, actor));
	}
}
void UniformObjective::cancel(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.move_pathRequestMaybeCancel(actor);
}
void UniformObjective::reset(Area& area, const ActorIndex actor)
{
	Actors& actors = area.getActors();
	actors.canReserve_clearAll(actor);
	actors.move_pathRequestMaybeCancel(actor);
	m_item.clear();
}
Point3D UniformObjective::getLocationOfItemInCuboid(Area& area, const Cuboid cuboid) const
{
	const ItemIndex item = const_cast<UniformObjective*>(this)->getItemAtLocation(area, cuboid);
	return item.empty() ? Point3D::null() : area.getItems().getLocation(item);
}
ItemIndex UniformObjective::getItemAtLocation(Area& area, const Cuboid cuboid)
{
	Items& items = area.getItems();
	const auto condition = [&](const ItemIndex item)
	{
		for(auto& element : m_elementsCopy)
			if(element.query(item, items))
				return true;
		return false;
	};
	return area.getSpace().item_getOneWithCondition(cuboid, condition);
}
void UniformObjective::select(Area& area, const ItemIndex item) { m_item.setIndex(item, area.getItems().m_referenceData); }
void UniformObjective::equip(Area& area, const ItemIndex item, const ActorIndex actor)
{
	Items& items = area.getItems();
	for(auto& element : m_elementsCopy)
		if(element.query(item, items))
		{
			if(items.getQuantity(item) >= element.m_quantity)
				std::erase(m_elementsCopy, element);
			else
				element.m_quantity -= items.getQuantity(item);
			break;
		}
	items.location_clear(item);
	Actors& actors = area.getActors();
	actors.equipment_add(actor, item);
	if(m_elementsCopy.empty())
		actors.objective_complete(actor, *this);
	else
	{
		reset(area, actor);
		execute(area, actor);
	}
}