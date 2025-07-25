 #include "unmount.h"
 #include "../actors/actors.h"
 #include "../area/area.h"
 #include "../space/space.h"
UnmountObjective::UnmountObjective(const Json& data, DeserializationMemo& deserializationMemo) :
	Objective(data, deserializationMemo),
	m_location(data["location"].get<Point3D>())
{ }
Json UnmountObjective::toJson() const
{
	Json data = Objective::toJson();
	data["location"] = m_location;
	return data;
}
void UnmountObjective::execute(Area& area, const ActorIndex& actor)
{
	Space& space = area.getSpace();
	Actors& actors = area.getActors();
	assert(actors.onDeck_getIsOnDeckOf(actor).isActor());
	const ActorIndex& mount = actors.onDeck_getIsOnDeckOf(actor).getActor();
	const ShapeId& shape = actors.getCompoundShape(actor);
	const Point3D& startingLocation = actors.getLocation(actor);
	if(m_location.empty())
	{
		for(const Point3D& point : actors.getAdjacentPoints(mount))
		{
			if(space.shape_canEnterCurrentlyFrom(m_location, shape, startingLocation, actors.getOccupied(actor)))
			{
				actors.mount_undo(actor, point, actors.getLocation(actor).getFacingTwords(point));
				actors.objective_complete(actor, *this);
				return;
			}
		}
		actors.objective_canNotCompleteObjective(actor, *this);
	}
	else
	{
		const Point3D& actorLocation = actors.getLocation(actor);
		const ShapeId& compoundShape = actors.getCompoundShape(mount);
		auto currentlyAdjacent = Shape::getPointsWhichWouldBeAdjacentAt(compoundShape, space, actorLocation, actors.getFacing(mount));
		if(currentlyAdjacent.contains(m_location))
		{

			const Facing4 facing = space.shape_canEnterEverWithAnyFacingReturnFacing(m_location, actors.getShape(actor), actors.getMoveType(actor));
			if(facing != Facing4::Null)
			{
				actors.mount_undo(actor, m_location, actorLocation.getFacingTwords(m_location));
				actors.objective_complete(actor, *this);
			}
			else
				actors.objective_canNotCompleteObjective(actor, *this);
		}
		else
			actors.move_setDestinationAdjacentToLocation(actor, m_location);
	}
}