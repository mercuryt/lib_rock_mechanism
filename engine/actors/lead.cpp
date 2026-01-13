#include "actors.h"
#include "../area/area.h"
#include "../space/space.h"
#include "../numericTypes/index.h"
#include "../geometry/setOfPointsHelper.h"
const SmallSet<Point3D>& Actors::lineLead_getPath(const ActorIndex& index) const
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	return m_leadFollowPath[index];
}
ShapeId Actors::lineLead_getLargestShape(const ActorIndex& index) const
{
	ShapeId output = m_shape[index];
	ActorOrItemIndex follower = index.toActorOrItemIndex();
	while(follower.exists())
	{
		ShapeId followerShape = follower.getShape(m_area);
		if(Shape::getCuboidsCount(output) < Shape::getCuboidsCount(followerShape))
			output = followerShape;
		follower = follower.getFollower(m_area);
	}
	return output;
}
MoveTypeId Actors::lineLead_getMoveType(const ActorIndex& index) const
{
	//TODO: iterate line and find most restrictive move type
	MoveTypeId output = m_moveType[index];
	return output;
}
Speed Actors::lineLead_getSpeedWithAddedMass(const ActorIndex& index, const Mass& mass) const
{
	auto actorsAndItems = lineLead_getAll(index);
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(m_area, actorsAndItems, Mass::create(0), Mass::create(0), mass);
}
Speed Actors::lineLead_getSpeedWithAddedMass(const SmallSet<ActorIndex>& indices, const Mass& mass) const
{
	std::vector<ActorOrItemIndex> vector;
	vector.resize(indices.size());
	for(const ActorIndex& index : indices)
		vector.push_back(ActorOrItemIndex::createForActor(index));
	return PortablesHelpers::getMoveSpeedForGroupWithAddedMass(m_area, vector, Mass::create(0), Mass::create(0), mass);
}
std::vector<ActorOrItemIndex> Actors::lineLead_getAll(const ActorIndex& index) const
{
	std::vector<ActorOrItemIndex> output;
	ActorOrItemIndex current = ActorOrItemIndex::createForActor(index);
	while(current.exists())
	{
		output.push_back(current);
		current = current.getFollower(m_area);
	}
	return output;
}
std::pair<Point3D, Facing4> Actors::lineLead_followerGetNextStep(const ActorOrItemIndex& follower, const SmallSet<Point3D>& path, const CuboidSet& occupiedByCurrentLeader) const
{
	assert(occupiedByCurrentLeader.exists());
	assert(follower.exists());
	assert(follower.isFollowing(m_area));
	const Space& space = m_area.getSpace();
	const Point3D currentLocation = follower.getLocation(m_area);
	auto indexOfCurrentLocation = path.maybeFindLastIndex(currentLocation);
	const bool isOnPathOrAdjacentToStart = indexOfCurrentLocation != -1 || (!path.empty() && path.back().isAdjacentTo(currentLocation));
	const ShapeId shape = follower.getShape(m_area);
	if(isOnPathOrAdjacentToStart)
	{
		if(indexOfCurrentLocation == -1)
			// The current position is not on the path but it is adjacent to the back.
			// Setting current location to path size acts like the location was found at one path the end of path.
			indexOfCurrentLocation = path.size();
		int indexOfCandidateLocation = indexOfCurrentLocation - 1;
		Point3D previous = currentLocation;
		// Step through the path from the current point onward untill we find one where we touch the current leader.
		for(; indexOfCandidateLocation != -1; --indexOfCandidateLocation)
		{
			Point3D candidate = path[indexOfCandidateLocation];
			Facing4 candidateFacing = previous.getFacingTwords(candidate);
			if(!space.shape_canFitEver(candidate, shape, candidateFacing))
				return {Point3D::null(), Facing4::Null};
			// TODO: this call contains some redundant logic with shape_canFitEver.
			const CuboidSet toOccupy = Shape::getCuboidsOccupiedAt(shape, space, candidate, candidateFacing);
			const bool isTouching = toOccupy.isTouching(occupiedByCurrentLeader);
			if(isTouching)
				return {candidate, candidateFacing};
			previous = candidate;
		}
	}
	else
	{
		// Not yet on path or adjacent to it's start, try and pick a next step which gets closer while maintaining touch with leader.
		Cuboid candidates = space.getAdjacentWithEdgeAndCornerAdjacent(currentLocation);
		Cuboid exclude{currentLocation, currentLocation};
		const Cuboid& boundry = space.boundry();
		const MoveTypeId& moveType = follower.getMoveType(m_area);
		for(Distance jumpDistance{1}; jumpDistance < 100; ++jumpDistance)
		{
			DistanceSquared closestToPathDistanceSquared = DistanceSquared::max();
			std::pair<Point3D, Facing4> output;
			// Iterate candidates lookin for one which the follower can enter, where it would be touching the current leader, and find the one nearest to any point on the path.
			for(const Point3D& candidate : candidates)
				if(
					!exclude.contains(candidate) &&
					space.shape_anythingCanEnterEver(candidate)
				)
				{
					const Facing4 facing = currentLocation.getFacingTwords(candidate);
					if(space.shape_canFitEver(candidate, shape, facing) && space.shape_moveTypeCanEnter(candidate, moveType))
					{
						// TODO: this call contains some redundant logic with shape_canFitEver.
						const CuboidSet toOccupy = Shape::getCuboidsOccupiedAt(shape, space, candidate, facing);
						const bool isTouching = toOccupy.isTouching(occupiedByCurrentLeader);
						if(isTouching)
						{
							DistanceSquared distanceSquaredToPath = path.empty() ? DistanceSquared{0} : setOfPointsHelper::distanceToClosestSquared(path, candidate);
							if(distanceSquaredToPath == 0)
								// Either the candidate is on the path or the path is empty. Either way this candidate is as good as any.
								return {candidate, facing};
							if(distanceSquaredToPath < closestToPathDistanceSquared)
							{
								closestToPathDistanceSquared = distanceSquaredToPath;
								output = {candidate, facing};
							}
						}
					}
				}
			if(output.first.exists())
				// At least one candidate was found. Return the candidate and facing nearest to the path.
				return output;
			if(exclude.contains(occupiedByCurrentLeader.boundry()))
				// We have already seached all reasonable candidates, break out of the loop and return null.
				break;
			// No valid candidate found, expand the search area.
			exclude = candidates;
			candidates.inflate({1});
			candidates = candidates.intersection(boundry);
		}

	}
	return {Point3D::null(), Facing4::Null};
}
bool Actors::lineLead_followersCanMoveEver(const ActorIndex& index) const
{
	assert(isLeading(index));
	assert(!isFollowing(index));
	const Space& space = m_area.getSpace();
	ActorOrItemIndex follower = getFollower(index);
	auto& path = lineLead_getPath(index);
	Point3D next;
	CuboidSet futureOccupiedForCurrentLeader = getOccupied(index);
	while(follower.exists())
	{
		if(futureOccupiedForCurrentLeader.isIntersectingOrAdjacentTo(follower.getOccupied(m_area)))
			return true;
		const auto& [location, facing] = lineLead_followerGetNextStep(follower, path, futureOccupiedForCurrentLeader);
		if(location.empty())
			return false;
		futureOccupiedForCurrentLeader = Shape::getCuboidsOccupiedAndAdjacentAt(follower.getShape(m_area), space, location, facing);
		follower = follower.getFollower(m_area);
	}
	return true;
}
bool Actors::lineLead_followersCanMoveCurrently(const ActorIndex& index) const
{
	assert(isLeading(index));
	assert(!isFollowing(index));
	const Space& space = m_area.getSpace();
	ActorOrItemIndex follower = getFollower(index);
	auto& path = lineLead_getPath(index);
	const CuboidSet& occupied = lineLead_getOccupiedCuboids(index);
	Point3D next;
	CuboidSet futureOccupiedForCurrentLeader = getOccupied(index);
	while(follower.exists())
	{
		if(futureOccupiedForCurrentLeader.isIntersectingOrAdjacentTo(follower.getOccupied(m_area)))
			// Leader and follower are intersecting after leader took a step, wait for leader to take another step.
			return true;
		const ShapeId& shape = follower.getShape(m_area);
		const auto& [location, facing] = lineLead_followerGetNextStep(follower, path, futureOccupiedForCurrentLeader);
		assert(location.exists());
		if(futureOccupiedForCurrentLeader.contains(location))
			// This one cannot advance because it's next step is occupied by it's leader even after the leader has advanced.
			// We return true because we still want the leader and everything ahead of it to advance, and we know that nothing after this ponit will be able to.
			return true;
		if(!space.shape_canEnterCurrentlyWithFacing(location, shape, facing, occupied))
			return false;
		futureOccupiedForCurrentLeader = Shape::getCuboidsOccupiedAndAdjacentAt(shape, space, location, facing);
		follower = follower.getFollower(m_area);
	}
	return true;
}
CuboidSet Actors::lineLead_getOccupiedCuboids(const ActorIndex& index) const
{
	CuboidSet output;
	assert(isLeading(index));
	assert(!isFollowing(index));
	ActorOrItemIndex follower = index.toActorOrItemIndex();
	while(follower.exists())
	{
		output.addSet(follower.getOccupied(m_area));
		follower = follower.getFollower(m_area);
	}
	return output;
}
bool Actors::lineLead_pathEmpty(const ActorIndex& index) const
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	return m_leadFollowPath[index].empty();
}
void Actors::lineLead_pushFront(const ActorIndex& index, const Point3D& point)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	assert(m_location[index] == point);
	m_leadFollowPath[index].insertFrontNonunique(point);
}
void Actors::lineLead_popBackUnlessOccupiedByFollower(const ActorIndex& index)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	ActorOrItemIndex follower = index.toActorOrItemIndex();
	while(follower.isLeading(m_area))
		follower = follower.getFollower(m_area);
	if(m_leadFollowPath[index].size() > 1 && follower.getLocation(m_area) == *(m_leadFollowPath[index].end() - 2))
		m_leadFollowPath[index].popBack();
}
void Actors::lineLead_clearPath(const ActorIndex& index)
{
	m_leadFollowPath[index].clear();
}
void Actors::lineLead_appendToPath(const ActorIndex& index, const Point3D& point, const Facing4& facing)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	if(m_leadFollowPath[index].empty())
		m_leadFollowPath[index].insert(m_location[index]);
	const Point3D& back = m_leadFollowPath[index].back();
	if(point == back)
		return;
	Space& space = m_area.getSpace();
	const ShapeId& shape = lineLead_getLargestShape(index);
	const MoveTypeId& moveType = lineLead_getMoveType(index);
	if(point.isAdjacentTo(back) && space.shape_shapeAndMoveTypeCanEnterEverFrom(back, shape, moveType, point))
		m_leadFollowPath[index].insert(point);
	else
	{
		constexpr bool anyOccupied = true;
		constexpr bool adjacent = false;
		auto path = m_area.m_hasTerrainFacades.getForMoveType(moveType).findPathToWithoutMemo<anyOccupied, adjacent>(point, facing, shape, back);
		// TODO: Can this fail?
		assert(!path.path.empty());
		assert(!path.path.contains(point));
		path.path.insert(point);
		m_leadFollowPath[index].insertAllNonunique(path.path);
	}
}
// TODO: very redundant with can move.
void Actors::lineLead_moveFollowers(const ActorIndex& index)
{
	assert(isLeading(index));
	assert(!isFollowing(index));
	const Space& space = m_area.getSpace();
	ActorOrItemIndex follower = getFollower(index);
	const SmallSet<Point3D>& path = lineLead_getPath(index);
	const CuboidSet& occupied = lineLead_getOccupiedCuboids(index);
	CuboidSet occupiedForCurrentLeader = getOccupied(index);
	// Leader has already moved, so it's future occupied is its current occupied.
	while(follower.exists())
	{
		if(occupiedForCurrentLeader.intersects(follower.getOccupied(m_area)))
			// Leader and follower are intersecting after leader took a step, wait for leader to take another step.
			return;
		const ShapeId& shape = follower.getShape(m_area);
		const auto& [location, facing] = lineLead_followerGetNextStep(follower, path, occupiedForCurrentLeader);
		assert(location.exists());
		if(occupiedForCurrentLeader.contains(location))
			// Next spot to occupy is already occupied by the current leader. Stop advancing here.
			return;
		assert(space.shape_canEnterCurrentlyWithFacing(location, shape, facing, occupied));
		follower.location_set(m_area, location, facing);
		occupiedForCurrentLeader = follower.getOccupied(m_area);
		follower = follower.getFollower(m_area);
	}
}