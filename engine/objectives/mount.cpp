 #include "mount.h"
 #include "station.h"
 #include "../actors/actors.h"
 #include "../area/area.h"
MountObjective::MountObjective(const Json& data, DeserializationMemo& deserializationMemo, Actors& actors) :
	Objective(data, deserializationMemo),
	m_pilot(data["pilot"].get<bool>())
{
	m_toMount.load(data["toMount"], actors.m_referenceData);
}
Json MountObjective::toJson() const
{
	Json output = Objective::toJson();
	output["toMount"] = m_toMount;
	output["pilot"] = m_pilot;
	return output;
}
void MountObjective::execute(Area& area, const ActorIndex& actor)
{
	Actors& actors = area.getActors();
	const ActorIndex& toMount = m_toMount.getIndex(actors.m_referenceData);
	if(actors.isAdjacentToActor(actor, toMount))
	{
		const Point3D& location = actors.mount_findLocationToMountOn(actor, toMount);
		if(location.empty())
		{
			// Cannot mount.
			actors.objective_canNotCompleteObjective(actor, *this);
			return;
		}
		actors.mount_do(actor, toMount, location, m_pilot);
		if(m_pilot)
			actors.objective_complete(actor, *this);
		else
		{
			std::unique_ptr<StationObjective> stationObjective = std::make_unique<StationObjective>(location);
			actors.objective_addTaskToStart(actor, std::move(stationObjective));
		}
	}
	else
	{
		// Get into position to mount.
		constexpr bool detour = false;
		constexpr bool unreserved = true;
		constexpr bool reserve = true;
		actors.move_setDestinationAdjacentToActor(actor, toMount, detour, unreserved, reserve);
	}
}