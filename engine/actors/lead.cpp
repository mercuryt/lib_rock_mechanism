#include "actors.h"
#include "index.h"
BlockIndices Actors::lineLead_getPath(const ActorIndex& index) const
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
OccupiedBlocksForHasShape Actors::lineLead_getOccupiedBlocks(const ActorIndex& index) const
{
	OccupiedBlocksForHasShape output;
	assert(isLeading(index));
	assert(!isFollowing(index));
	ActorOrItemIndex follower = index.toActorOrItemIndex();
	while(follower.exists())
	{
		for(BlockIndex block : follower.getBlocks(m_area))
			output.addNonunique(block);
		follower = follower.getFollower(m_area);
	}
	output.removeDuplicates();
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
	m_leadFollowPath[index].pushFrontNonunique(block);
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
void Actors::lineLead_appendToPath(const ActorIndex& index, const BlockIndex& block)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	if(m_leadFollowPath[index].empty())
		m_leadFollowPath[index].add(m_location[index]);
	m_leadFollowPath[index].add(block);
}
