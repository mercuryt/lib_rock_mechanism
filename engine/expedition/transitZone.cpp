#include "transitZone.h"
#include "../geometry/cuboidSet.h"
#include "../geometry/cuboidSetHelper.hpp"
#include "../space/space.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../items/items.h"
#include "../definitions/moveType.h"
#include "../numericTypes/actorOrItemId.h"
#include "../simulation/hasAreas.h"
CuboidSet expeditionTransitZone::makeTransitZone(const Area& area, Facing6 facing)
{
	CuboidSet candidates;
	const Space& space = area.getSpace();
	Cuboid boundry = space.boundry();
	candidates.add(boundry.getFace(facing));
	space.move_removeUnenterableFrom(candidates);
	return candidates;
}
Point3D expeditionTransitZone::findArrivalPointInZone(const Area& toArea, const Area& fromArea, Facing6 facing, ActorIndex leader, MoveTypeId expeditionMoveType, const CuboidSet& zone, Point3D center)
{
	// Finds a location for a single actor that also considers spacing for any mount / vehicle / passengers / followers.
	const Space& space = toArea.getSpace();
	const Actors& actors = fromArea.getActors();
	Facing4 facing4 = facing6ToFacing4(facing);
	OffsetCuboid boundry = actors.lineLead_getHypotheticalStraightLineBoundry(leader, facing4);
	Distance offset = Distance::create(boundry.m_low.y().get() * -1);
	return cuboidSetHelper::nearestPointToWithConiditon(zone, center, [&space, expeditionMoveType, boundry, facing4, offset](Point3D point){
		// Move point forward so entire boundry is in area bounds.
		OffsetCuboid shiftedBoundry = boundry.relativeToOffset(point.shift(facing4, offset));
		if(!space.boundry().contains(shiftedBoundry))
			return false;
		Cuboid cuboidBoundry = Cuboid::create(shiftedBoundry);
		if(space.move_containsUnenterable(cuboidBoundry))
			return false;
		if(MoveType::getSurface(expeditionMoveType))
			cuboidBoundry = cuboidBoundry.getFaceBelow();
		if(space.move_containsUnenterableForMoveType(cuboidBoundry, expeditionMoveType))
			return false;
		return true;
	});
}
std::pair<ActorOrItemIndex, SmallSet<ActorOrItemIndex>> expeditionTransitZone::arriveInZone(Area& toArea, Area& fromArea, Point3D location, Facing4 facing, ActorOrItemIndex index)
{
	// TODO: update leader and follower.
	Actors& actors = fromArea.getActors();
	Items& items = fromArea.getItems();
	ShapeId shape = index.getCompoundShape(fromArea);
	Point3D oldLocation = index.getLocation(fromArea);
	Facing4 oldFacing = index.getFacing(fromArea);
	ActorOrItemIndex follower = index.getFollower(fromArea);
	SmallSet<ActorOrItemIndex> newIndices;
	ActorOrItemIndex baseLevelNewIndex;
	ActorOrItemReference followerRef = follower.exists() ? follower.toReference(fromArea) : ActorOrItemReference();
	// Find and store new locations for anything on deck, recursive.
	struct NewLocations
	{
		Point3D location;
		ActorOrItemReference toMove;
		ActorId mountedOn; // Vehicles have decks and thus can reconstruct onDeck implicitly via relative position, but actors must be mounted explicitly.
		Facing4 facing;
	};
	std::vector<NewLocations> newLocations;
	struct OpenList
	{
		ActorOrItemIndex arriving;
		ActorId mountedOn;
	};
	std::vector<OpenList> openList;
	openList.emplace_back(index, ActorId::null());
	while(!openList.empty())
	{
		auto [arriving, mountedOn] = openList.back();
		openList.pop_back();
		Offset3D offset = oldLocation.offsetTo(arriving.getLocation(fromArea));
		Point3D newLocation = Point3D::create(location.applyOffset(offset));
		Facing4 newFacing = (Facing4)(((int)oldFacing - (int)oldFacing) + (int)facing);
		newLocations.emplace_back(newLocation, arriving.getReference(fromArea), mountedOn, newFacing);
		ActorId mountedOn2 = arriving.isActor() ? actors.getId(arriving.getActor()) : ActorId::null();
		for(ActorOrItemIndex onDeckSingle : arriving.getOnDeck(fromArea))
			openList.emplace_back(onDeckSingle, mountedOn2);
	}
	// Move to new locations in the same order as the actors / items on deck were found.
	Actors& newActors  = toArea.getActors();
	for(auto [newLocation, actorOrItemReference, mountedOn, newFacing] : newLocations)
	{
		ActorOrItemIndex oldIndex = actorOrItemReference.getIndexPolymorphic(actors.m_referenceData, items.m_referenceData);
		ActorOrItemIndex newIndex = oldIndex.moveTo(fromArea, toArea);
		newIndices.insert(newIndex);
		if(baseLevelNewIndex.empty())
			// First to be moved is 'base level' actor or item, store for setting lead/follow.
			baseLevelNewIndex = newIndex;
		newIndex.location_set(toArea, newLocation, newFacing);
		// Reconstruct mounted on and item pilot relationships.
		// Item monuted on is intrinsic from setLocation due to Deck.
		// Actor pilot is stored only in Actors::m_isPilot.
		if(mountedOn.exists())
		{
			// Record mounted on actor.
			ActorIndex mountedOnIndex = toArea.m_simulation.m_actors.getIndexForId(mountedOn);
			newActors.mount_set(newIndex.getActor(), mountedOnIndex);
		}
		else if(newIndex.isActor() && newActors.mount_isPilot(newIndex.getActor()))
		{
			// Record piloting vehicle.
			ItemIndex onDeckOf = newActors.onDeck_getIsOnDeckOf(newIndex.getActor()).getItem();
			newActors.pilotItem_set(newIndex.getActor(), onDeckOf);
		}
	}
	// Recursive for followers.
	if(followerRef.exists())
	{
		Facing4 inverseFacing = flipFacing4(facing);
		ShapeId followerShape = follower.getCompoundShape(fromArea);
		location = Point3D::create(location.shift(inverseFacing, Shape::getDistanceFromBack(shape) + Shape::getDistanceFromFront(followerShape) + 1));
		auto [newFollowIndex, otherFollowIndices] = arriveInZone(toArea, fromArea, location, facing, follower);
		newFollowIndex.followPolymorphic(toArea, baseLevelNewIndex);
	}
	return {baseLevelNewIndex, newIndices};
}
ActorOrItemIndex expeditionTransitZone::exitArea(Expedition& expedition, Area& fromArea, ActorOrItemIndex index)
{
	Area& toArea = fromArea.m_simulation.m_hasAreas->getById(expedition.area);
	Actors& fromActors = fromArea.getActors();
	Actors& toActors = toArea.getActors();
	// If the index is a pilot then have the mount exit the area instead.
	if(index.isActor() && fromActors.mount_isPilot(index.getActor()))
		return exitArea(expedition, fromArea, fromArea.getActors().getIsPiloting(index.getActor()));
	// Store ondeck and pilot relationships by id so they can be reconstructed after moving.
	// Arrive in zone reconstructs the onDeck relationships explicitly by location.
	SmallMap<ActorOrItemId, SmallSet<ActorOrItemId>> hasOnDeck;
	SmallSet<ActorOrItemReference> toExit;
	ActorOrItemIndex followerIndex = index.getFollower(fromArea);
	ActorOrItemIndex output;
	ActorOrItemReference followerRef = followerIndex.exists() ? followerIndex.toReference(fromArea) : ActorOrItemReference();
	// Collect onDeck recursively, record onDeck and pilot relationships by id.
	// Ids are the only reference type valid across the Area split.
	SmallSet<ActorOrItemIndex> openList;
	openList.insert(index);
	while(!openList.empty())
	{
		ActorOrItemIndex current = openList.back();
		openList.popBack();
		toExit.insert(current.toReference(fromArea));
		ActorOrItemId id = current.getId(fromArea);
		// Record onDeck.
		const auto& onDeck = current.getOnDeck(fromArea);
		if(!onDeck.empty())
		{
			SmallSet<ActorOrItemId>& ids = hasOnDeck.getOrCreate(id);
			for(ActorOrItemIndex onDeckSingle : onDeck)
			{
				openList.insert(onDeckSingle);
				ids.insert(onDeckSingle.getId(fromArea));
			}
		}
	}
	// Move data.
	for(auto iter = toExit.m_data.rbegin(); iter != toExit.m_data.rend(); ++iter)
	{
		ActorOrItemIndex onDeckIndex = iter->getIndexPolymorphic(fromActors.m_referenceData, fromArea.getItems().m_referenceData);
		ActorOrItemIndex newIndex = onDeckIndex.moveTo(fromArea, toArea);
		// The last new index will belong to the base level vehicle or actor.
		output = newIndex;
		// Remove from waiting for.
		if(newIndex.isActor())
			expedition.waitingFor.erase(toActors.getId(newIndex.getActor()));
	}
	// Reconstruct onDeck and is pilot.
	for(auto [hasDeckId, onDeckIds] : hasOnDeck)
	{
		ActorOrItemIndex newHasDeckIndex = hasDeckId.getIndex(toArea.m_simulation);
		if(newHasDeckIndex.isActor())
		{
			for(ActorOrItemId id : onDeckIds)
			{
				// Only actors can be mounted on other actors.
				ActorIndex newOnDeckIndex = id.getIndex(toArea.m_simulation).getActor();
				toActors.mount_set(newOnDeckIndex, newHasDeckIndex.getActor());
			}
		}
		else
		{
			for(ActorOrItemId id : onDeckIds)
			{
				ActorOrItemIndex newOnDeckIndex = id.getIndex(toArea.m_simulation);
				newOnDeckIndex.setOnDeck(toArea, newHasDeckIndex);
				if(newOnDeckIndex.isActor() && toActors.mount_isPilot(newOnDeckIndex.getActor()))
					newHasDeckIndex.setPilot(toArea, newOnDeckIndex.getActor());
			}
		}
	}
	// Recursive for follower.
	if(followerRef.exists())
	{
		ActorOrItemIndex follower = followerRef.getIndexPolymorphic(fromActors.m_referenceData, fromArea.getItems().m_referenceData);
		ActorOrItemIndex newIndex = expeditionTransitZone::exitArea(expedition, fromArea, follower);
		newIndex.followPolymorphic(toArea, output);
	}
	return output;
}