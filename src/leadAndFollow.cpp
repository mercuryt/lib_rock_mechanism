#include "leadAndFollow.h"
#include "block.h"
void CanLead::onMove()
{
	if(!isLeading())
		return;
	assert(getLocationQueue().size() > 1);
	// When leader moves add its new location to the front of the location queue.
	if(!m_hasShape.m_canFollow.isFollowing())
		m_locationQueue.push_front(m_hasShape.m_location);
	HasShape* nextInLine = &m_canFollow->m_hasShape;
	nextInLine->m_canFollow.tryToMove();
}
bool CanLead::isFollowerKeepingUp() const { return m_hasShape.isAdjacentTo(m_canFollow->m_hasShape); }
bool CanLead::isLeading() const { return m_canFollow != nullptr; }
bool CanLead::isLeading(HasShape& hasShape) const  { return m_canFollow != nullptr && &hasShape.m_canFollow == m_canFollow; }
HasShape& CanLead::getFollower() { return m_canFollow->m_hasShape; }
const HasShape& CanLead::getFollower() const { return m_canFollow->m_hasShape; }
// Class method.
uint32_t CanLead::getMoveSpeedForGroupWithAddedMass(std::vector<const HasShape*>& actorsAndItems, uint32_t addedRollingMass, uint32_t addedDeadMass)
{
	uint32_t rollingMass = addedRollingMass;
	uint32_t deadMass = addedDeadMass;
	uint32_t carryMass = 0;
	uint32_t lowestMoveSpeed = 0;
	for(const HasShape* hasShape : actorsAndItems)
	{
		if(hasShape->isItem())
		{
			Mass mass = static_cast<const Item&>(*hasShape).getMass();
			static const MoveType& roll = MoveType::byName("roll");
			if(hasShape->getMoveType() == roll)
				rollingMass += mass;
			else
				deadMass += mass;
		}
		else
		{
			assert(hasShape->isActor());
			const Actor& actor = static_cast<const Actor&>(*hasShape);
			if(actor.m_canMove.canMove())
			{
				carryMass += actor.m_attributes.getUnencomberedCarryMass();
				uint32_t moveSpeed = actor.m_attributes.getMoveSpeed();
				lowestMoveSpeed = lowestMoveSpeed == 0 ? moveSpeed : std::min(lowestMoveSpeed, moveSpeed);
			}
			else
				deadMass += actor.getMass();
		}
	}
	assert(lowestMoveSpeed != 0);
	uint32_t totalMass = deadMass + (rollingMass * Config::rollingMassModifier);
	if(totalMass <= carryMass)
		return lowestMoveSpeed;
	float ratio = totalMass / carryMass;
	if(ratio > Config::massCarryMaximimMovementRatio)
		return 0;
	return lowestMoveSpeed / ratio;
}
uint32_t CanLead::getMoveSpeed() const
{
	assert(!m_hasShape.m_canFollow.isFollowing());
	assert(m_hasShape.m_canLead.isLeading());
	HasShape* hasShape = &m_hasShape;
	std::vector<const HasShape*> actorsAndItems;
	while(true)
	{
		actorsAndItems.push_back(hasShape);
		if(!hasShape->m_canLead.isLeading())
			break;
		hasShape = &hasShape->m_canLead.getFollower();
	}
	return getMoveSpeedForGroupWithAddedMass(actorsAndItems, 0);
}
std::deque<Block*>& CanLead::getLocationQueue()
{
	return m_hasShape.m_canFollow.getLineLeader().m_canLead.m_locationQueue;
}
CanFollow::CanFollow(HasShape& a) : m_hasShape(a), m_canLead(nullptr), m_event(a.getEventSchedule()) { }
void CanFollow::follow(CanLead& canLead, bool doAdjacentCheck)
{
	assert(m_canLead == nullptr);
	assert(canLead.m_canFollow == nullptr);
	if(doAdjacentCheck)
		assert(m_hasShape.isAdjacentTo(canLead.m_hasShape));
	canLead.m_canFollow = this;
	m_canLead = &canLead;
	if(m_hasShape.isItem())
		m_hasShape.setStatic(false);
	std::deque<Block*>& locationQueue = canLead.getLocationQueue();
	if(locationQueue.empty())
	{
		assert(!canLead.m_hasShape.m_canFollow.isFollowing());
		locationQueue.push_back(canLead.m_hasShape.m_location);
	}
	locationQueue.push_back(m_hasShape.m_location);
}
void CanFollow::unfollow()
{
	assert(m_canLead != nullptr);
	m_canLead->m_canFollow = nullptr;
	m_canLead->m_locationQueue.clear();
	m_canLead = nullptr;
	if(m_hasShape.isItem())
		m_hasShape.setStatic(true);
}
void CanFollow::unfollowIfFollowing()
{
	if(m_canLead != nullptr)
		unfollow();
}
void CanFollow::disband(bool taskComplete)
{
	HasShape* leader = &getLineLeader();
	leader->m_canLead.m_locationQueue.clear();
	while(leader->m_canLead.isLeading())
	{
		if(taskComplete && leader->isActor())
			static_cast<Actor*>(leader)->m_hasObjectives.taskComplete();
		leader = &leader->m_canLead.m_canFollow->m_hasShape;
		leader->m_canFollow.unfollow();
	}
}
void CanFollow::tryToMove()
{
	assert(isFollowing());
	std::deque<Block*>& locationQueue = m_hasShape.m_canLead.getLocationQueue();
	// Find the position in the location queue where the next shape in the line currently is.
	auto found = std::ranges::find(locationQueue, m_hasShape.m_location);
	// If this position is in the front of location queue do nothing, wait for leader to cross over.
	if(found == locationQueue.begin())
		return;
	// Find the next position after that one and try to move follower into it.
	Block& block = **(--found);
	if(!block.m_hasShapes.anythingCanEnterEver() || !block.m_hasShapes.canEnterEverFrom(m_hasShape, *m_hasShape.m_location))
		// Shape can no longer enter this location, following path is imposible, disband.
		disband(true);
	else if(block.m_hasShapes.canEnterCurrentlyFrom(m_hasShape, *m_hasShape.m_location))
	{
		// setLocation calls CanLead::onMove which calls this method on it's follower, so this call is recursive down the line.
		m_hasShape.setLocation(block);
		// Remove from the end of the location queue if this is the last shape in line.
		if(!m_hasShape.m_canLead.isLeading())
			locationQueue.pop_back();
	}
	else
		m_event.schedule(m_hasShape);
}
HasShape& CanFollow::getLineLeader()
{
	if(!isFollowing())
		return m_hasShape;
	else
		return m_canLead->m_hasShape.m_canFollow.getLineLeader();
}
CanFollowEvent::CanFollowEvent(HasShape& hasShape) : ScheduledEventWithPercent(hasShape.getSimulation(), Config::stepsToDelayBeforeTryingAgainToFollowLeader), m_hasShape(hasShape) { }
void CanFollowEvent::execute() { m_hasShape.m_canFollow.tryToMove(); }
void CanFollowEvent::clearReferences() { m_hasShape.m_canFollow.m_event.clearPointer(); }
