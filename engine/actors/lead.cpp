#include "actors.h"
BlockIndices Actors::lineLead_getPath(ActorIndex index) const
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	return m_leadFollowPath[index];
}
bool Actors::lineLead_pathEmpty(ActorIndex index) const
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	return m_leadFollowPath[index].empty();
}
void Actors::lineLead_clearPath(ActorIndex index)
{
	assert(!isFollowing(index));
	assert(isLeading(index));
	m_leadFollowPath[index].clear();
}
