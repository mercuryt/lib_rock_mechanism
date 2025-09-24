#include "actors.h"
#include "area/area.h"
#include "space/space.h"
#include "numericTypes/index.h"
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
bool Actors::lineLead_followersCanMoveEver(const ActorIndex& index) const
{
	assert(isLeading(index));
	assert(!isFollowing(index));
	const Space& space = m_area.getSpace();
	ActorOrItemIndex follower = getFollower(index);
	auto& path = lineLead_getPath(index);
	Point3D next;
	CuboidSet futureOccupiedForCurrentLeader = getOccupied(index);
	const CuboidSet& occupied = lineLead_getOccupiedCuboids(index);
	while(follower.exists())
	{
		const Point3D& location = follower.getLocation(m_area);
		auto lastIndexForLocation = path.maybeFindLastIndex(location);
		if(lastIndexForLocation  > 0)
		{
			// Follower is on path. Get the next step and check it for validity.
			next = path[lastIndexForLocation - 1];
			if(!space.shape_shapeAndMoveTypeCanEnterEverFrom(next, follower.getShape(m_area), follower.getMoveType(m_area), location))
				next.clear();
		}
		if(next.empty())
		{
			// Either the follower is not on the path yet or it can't enter the next space. Find another location for it which touches the future location of it's leader.
			const ShapeId& shape = follower.getShape(m_area);
			const Cuboid candidates = space.getAdjacentWithEdgeAndCornerAdjacent(location);
			const MoveTypeId& moveType = follower.getMoveType(m_area);
			for(const Point3D& point : candidates)
			{
				if(point == location)
					continue;
				const Facing4 facing = location.getFacingTwords(point);
				if(
					space.shape_anythingCanEnterEver(point) &&
					space.shape_moveTypeCanEnter(point, moveType) &&
					space.shape_shapeAndMoveTypeCanEnterEverFrom(point, shape, moveType, location)
				)
				{
					const CuboidSet toOccupy = Shape::getCuboidsOccupiedAt(follower.getShape(m_area), space, point, facing);
					if( toOccupy.isTouching(futureOccupiedForCurrentLeader))
						next = point;
				}
			}
			if(next.empty())
				return false;
		}
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
		const Point3D& location = follower.getLocation(m_area);
		auto lastIndexForLocation = path.maybeFindLastIndex(location);
		if(lastIndexForLocation  > 0)
		{
			// Follower is on path. Get the next step and check it for validity.
			next = path[lastIndexForLocation - 1];
			if(!space.shape_canEnterCurrentlyFrom(next, follower.getShape(m_area), location, occupied))
				next.clear();
		}
		if(next.empty())
		{
			const ShapeId& shape = follower.getShape(m_area);
			const Cuboid candidates = space.getAdjacentWithEdgeAndCornerAdjacent(location);
			const MoveTypeId& moveType = follower.getMoveType(m_area);
			for(const Point3D& point : candidates)
			{
				if(point == location)
					continue;
				const Facing4 facing = location.getFacingTwords(point);
				if(
					space.shape_anythingCanEnterEver(point) &&
					space.shape_moveTypeCanEnter(point, moveType) &&
					space.shape_shapeAndMoveTypeCanEnterEverAndCurrentlyFrom(point, shape, moveType, location, occupied)
				)
				{
					const CuboidSet toOccupy = Shape::getCuboidsOccupiedAt(follower.getShape(m_area), space, point, facing);
					if( toOccupy.isTouching(futureOccupiedForCurrentLeader))
						next = point;
				}
			}
			if(next.empty())
				return false;
		}
		futureOccupiedForCurrentLeader = getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(index, next, location.getFacingTwords(next));
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
	auto& occupiedByRearmostFollower = follower.getOccupied(m_area);
	while(!occupiedByRearmostFollower.contains(m_leadFollowPath[index].back()))
		m_leadFollowPath[index].popBack();
	assert(m_leadFollowPath[index].size() >= 2);
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
	// Leader has already moved, so it's future occupied is its current occupied.
	CuboidSet futureOccupiedForCurrentLeader = getOccupied(index);
	while(follower.exists())
	{
		const Point3D& location = follower.getLocation(m_area);
		auto lastIndexForLocation = path.maybeFindLastIndex(location);
		Point3D next;
		if(lastIndexForLocation > 0)
			next = path[lastIndexForLocation - 1];
		else if(!path.empty())
			next = path.back();
		const ShapeId& followerShape = follower.getShape(m_area);
		if(
			!next.isAdjacentTo(location) ||
			!space.shape_canFitEver(next, followerShape, location.getFacingTwords(next)) ||
			!space.shape_canEnterCurrentlyFrom(next, followerShape, location, occupied)
		)
		{
			next.clear();
			// Either the follower is not on the path yet or it can't enter the next space. Find another location for it which touches the future location of it's leader.
			const MoveTypeId& moveType = follower.getMoveType(m_area);
			const ShapeId& shape = follower.getShape(m_area);
			Cuboid candidates = space.getAdjacentWithEdgeAndCornerAdjacent(location);
			Distance closestDistance = Distance::max();
			for(const Point3D& point : candidates)
			{
				const Facing4 facing = location.getFacingTwords(point);
				if(
					!space.shape_anythingCanEnterEver(point) ||
					!space.shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(point, shape, moveType, facing, occupied)
				)
					continue;
				const CuboidSet cuboids = Shape::getCuboidsOccupiedAt(follower.getShape(m_area), space, point, facing);
				if(!cuboids.isTouching(futureOccupiedForCurrentLeader))
					continue;
				Distance pointDistance = point.distanceTo(path.back());
				if(closestDistance > pointDistance)
				{
					closestDistance = pointDistance;
					next = point;
				}
			}
			assert(next.exists());
		}
		follower.location_set(m_area, next, location.getFacingTwords((next)));
		// This is a redundant calculation which has already been done in location_set.
		futureOccupiedForCurrentLeader = getCuboidsWhichWouldBeOccupiedAtLocationAndFacing(index, next, location.getFacingTwords(next));
		follower = follower.getFollower(m_area);
	}
}