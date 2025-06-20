#include "actors.h"
#include "area/area.h"
#include "blocks/blocks.h"
#include "numericTypes/index.h"
const SmallSet<BlockIndex>& Actors::lineLead_getPath(const ActorIndex& index) const
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
		if(Shape::getPositions(output).size() < Shape::getPositions(followerShape).size())
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
	const Blocks& blocks = m_area.getBlocks();
	ActorOrItemIndex follower = getFollower(index);
	auto& path = lineLead_getPath(index);
	while(follower.exists())
	{
		const BlockIndex& location = follower.getLocation(m_area);
		auto index = path.findLastIndex(location);
		assert(index != path.size());
		if(index != 0)
		{
			const BlockIndex& next = path[index - 1];
			if(!blocks.shape_anythingCanEnterEver(next))
				return false;
			const Facing4& facing = blocks.facingToSetWhenEnteringFrom(next, location);
			if(!blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(next, follower.getShape(m_area), follower.getMoveType(m_area), facing))
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
	const Blocks& blocks = m_area.getBlocks();
	ActorOrItemIndex follower = getFollower(index);
	auto& path = lineLead_getPath(index);
	const auto& occupied = lineLead_getOccupiedBlocks(index);
	while(follower.exists())
	{
		const BlockIndex& location = follower.getLocation(m_area);
		auto index = path.findLastIndex(location);
		assert(index != path.size());
		if(index != 0)
		{
			const BlockIndex& next = path[index - 1];
			if(!blocks.shape_canEnterCurrentlyFrom(next, follower.getShape(m_area), location, occupied))
				return false;
		}
		follower = follower.getFollower(m_area);
	}
	return true;
}
OccupiedBlocksForHasShape Actors::lineLead_getOccupiedBlocks(const ActorIndex& index) const
{
	OccupiedBlocksForHasShape output;
	assert(isLeading(index));
	assert(!isFollowing(index));
	ActorOrItemIndex follower = index.toActorOrItemIndex();
	while(follower.exists())
	{
		for(BlockIndex block : follower.getBlocks(m_area))
			output.insertNonunique(block);
		follower = follower.getFollower(m_area);
	}
	output.makeUnique();
	return output;
}
bool Actors::lineLead_pathEmpty(const ActorIndex& index) const
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	return m_leadFollowPath[index].empty();
}
void Actors::lineLead_pushFront(const ActorIndex& index, const BlockIndex& block)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	assert(m_location[index] == block);
	m_leadFollowPath[index].insertFrontNonunique(block);
}
void Actors::lineLead_popBackUnlessOccupiedByFollower(const ActorIndex& index)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	ActorOrItemIndex follower = index.toActorOrItemIndex();
	while(follower.isLeading(m_area))
		follower = follower.getFollower(m_area);
	auto& blocksOfRearmostFollower = follower.getBlocks(m_area);
	while(!blocksOfRearmostFollower.contains(m_leadFollowPath[index].back()))
		m_leadFollowPath[index].popBack();
	assert(m_leadFollowPath[index].size() >= 2);
}
void Actors::lineLead_clearPath(const ActorIndex& index)
{
	m_leadFollowPath[index].clear();
}
void Actors::lineLead_appendToPath(const ActorIndex& index, const BlockIndex& block, const Facing4& facing)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	if(m_leadFollowPath[index].empty())
		m_leadFollowPath[index].insert(m_location[index]);
	const BlockIndex& back = m_leadFollowPath[index].back();
	if(block == back)
		return;
	Blocks& blocks = m_area.getBlocks();
	if(blocks.isAdjacentToIncludingCornersAndEdges(block, back))
		m_leadFollowPath[index].insert(block);
	else
	{
		const MoveTypeId& moveType = lineLead_getMoveType(index);
		const ShapeId& shape = lineLead_getLargestShape(index);
		auto path = m_area.m_hasTerrainFacades.getForMoveType(moveType).findPathToWithoutMemo(block, facing, shape, back);
		// TODO: Can this fail?
		assert(!path.path.empty());
		assert(!path.path.contains(block));
		path.path.insert(block);
		m_leadFollowPath[index].insertAllNonunique(path.path);
	}
}
void Actors::lineLead_moveFollowers(const ActorIndex& index)
{
	//TODO: This could be done better by walking the followers and path simultaniously.
	assert(isLeading(index));
	assert(!isFollowing(index));
	const Blocks& blocks = m_area.getBlocks();
	ActorOrItemIndex follower = getFollower(index);
	auto& path = lineLead_getPath(index);
	const auto& occupied = lineLead_getOccupiedBlocks(index);
	while(follower.exists())
	{
		const BlockIndex& location = follower.getLocation(m_area);
		auto found = path.find(location);
		assert(found != path.end());
		assert(found != path.begin());
		const BlockIndex& next = *(--found);
		if(follower.getLeader(m_area).occupiesBlock(m_area, next))
			return;
		follower.location_set(m_area, next, blocks.facingToSetWhenEnteringFrom(next, location));
		follower = follower.getFollower(m_area);
	}
}