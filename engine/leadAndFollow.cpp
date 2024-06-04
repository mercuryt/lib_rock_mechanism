#include "leadAndFollow.h"
#include "actor.h"
#include "area.h"
#include "config.h"
#include "item.h"
#include "deserializationMemo.h"
#include "simulation.h"
void CanLead::load(const Json& data, DeserializationMemo& deserializationMemo)
{

	m_canFollow = data.contains("canFollowItem") ? 
		&deserializationMemo.itemReference(data["canFollowItem"]).m_canFollow:
		&deserializationMemo.actorReference(data["canFollowActor"]).m_canFollow;
	if(data.contains("locationQueue"))
		for(const Json& block: data["locationQueue"])
			m_locationQueue.push_back(block.get<BlockIndex>());
}
Json CanLead::toJson() const
{
	Json data;
	if(m_canFollow)
	{
		if(m_canFollow->m_hasShape.isItem())
			data["canFollowItem"] = static_cast<Item&>(m_canFollow->m_hasShape).m_id;
		else
		{
			assert(m_canFollow->m_hasShape.isActor());
			data["canFollowActor"] = static_cast<Actor&>(m_canFollow->m_hasShape).m_id;
		}
	}
	if(!m_locationQueue.empty())
		data["locationQueue"] = m_locationQueue;
	return data;
}
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
bool CanLead::isFollowerKeepingUp() const 
{ 
	return m_hasShape.isAdjacentTo(m_canFollow->m_hasShape); 
}
bool CanLead::isLeading() const { return m_canFollow != nullptr; }
bool CanLead::isLeading(HasShape& hasShape) const  { return m_canFollow != nullptr && &hasShape.m_canFollow == m_canFollow; }
HasShape& CanLead::getFollower() { return m_canFollow->m_hasShape; }
const HasShape& CanLead::getFollower() const { return m_canFollow->m_hasShape; }
// Class method.
Speed CanLead::getMoveSpeedForGroupWithAddedMass(std::vector<const HasShape*>& actorsAndItems, Mass addedRollingMass, Mass addedDeadMass)
{
	Mass rollingMass = addedRollingMass;
	Mass deadMass = addedDeadMass;
	Mass carryMass = 0;
	Speed lowestMoveSpeed = 0;
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
				Speed moveSpeed = actor.m_attributes.getMoveSpeed();
				lowestMoveSpeed = lowestMoveSpeed == 0 ? moveSpeed : std::min(lowestMoveSpeed, moveSpeed);
			}
			else
				deadMass += actor.getMass();
		}
	}
	assert(lowestMoveSpeed != 0);
	Mass totalMass = deadMass + (rollingMass * Config::rollingMassModifier);
	if(totalMass <= carryMass)
		return lowestMoveSpeed;
	float ratio = (float)carryMass / (float)totalMass;
	if(ratio < Config::minimumOverloadRatio)
		return 0;
	return std::ceil(lowestMoveSpeed * ratio * ratio);
}
Speed CanLead::getMoveSpeed() const
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
std::deque<BlockIndex>& CanLead::getLocationQueue()
{
	return m_hasShape.m_canFollow.getLineLeader().m_canLead.m_locationQueue;
}
CanFollow::CanFollow(HasShape& a, Simulation& s) : m_event(s.m_eventSchedule), m_hasShape(a) { }
void CanFollow::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("canLeadItem"))
		m_canLead = &deserializationMemo.itemReference(data["canLeadItem"]).m_canLead;
	else if(data.contains("canLeadActor"))
		m_canLead = &deserializationMemo.itemReference(data["canLeadActor"]).m_canLead;
	if(data.contains("eventStart"))
		m_event.schedule(m_hasShape, data["eventStart"].get<Step>());
}
Json CanFollow::toJson() const
{
	Json data;
	if(m_canLead)
	{
		if(m_canLead->m_hasShape.isItem())
			data["canLeadItem"] = static_cast<Item&>(m_canLead->m_hasShape).m_id;
		else
		{
			assert(m_canLead->m_hasShape.isActor());
			data["canLeadActor"] = static_cast<Actor&>(m_canLead->m_hasShape).m_id;
		}
	}
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
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
	std::deque<BlockIndex>& locationQueue = canLead.getLocationQueue();
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
void CanFollow::maybeDisband()
{
	if(isFollowing() || m_hasShape.m_canLead.isLeading())
		disband();
}
void CanFollow::disband()
{
	HasShape* leader = &getLineLeader();
	leader->m_canLead.m_locationQueue.clear();
	while(leader->m_canLead.isLeading())
	{
		leader = &leader->m_canLead.m_canFollow->m_hasShape;
		assert(leader->m_canLead.m_locationQueue.empty());
		leader->m_canFollow.unfollow();
	}
}
void CanFollow::tryToMove()
{
	assert(isFollowing());
	std::deque<BlockIndex>& locationQueue = m_hasShape.m_canLead.getLocationQueue();
	// Find the position in the location queue where the next shape in the line currently is.
	auto found = std::ranges::find(locationQueue, m_hasShape.m_location);
	// If this position is in the front of location queue do nothing, wait for leader to cross over.
	if(found == locationQueue.begin())
		return;
	// Find the next position after that one and try to move follower into it.
	BlockIndex block = *(--found);
	Blocks& blocks = m_hasShape.m_area->getBlocks();
	if(!blocks.shape_anythingCanEnterEver(block) || !blocks.shape_canEnterEverFrom(block, m_hasShape, m_hasShape.m_location))
		// Shape can no longer enter this location, following path is imposible, disband.
		disband();
	else if(blocks.shape_canEnterCurrentlyFrom(block, m_hasShape, m_hasShape.m_location))
	{
		// setLocation calls CanLead::onMove which calls this method on it's follower, so this call is recursive down the line.
		m_hasShape.setLocation(block, m_hasShape.m_area);
		// Remove from the end of the location queue if this is the last shape in line.
		if(!m_hasShape.m_canLead.isLeading())
			locationQueue.pop_back();
	}
	else
		//Cannot follow currently, schedule a retry.
		m_event.schedule(m_hasShape);
}
HasShape& CanFollow::getLineLeader()
{
	if(!isFollowing())
		return m_hasShape;
	else
		return m_canLead->m_hasShape.m_canFollow.getLineLeader();
}
CanFollowEvent::CanFollowEvent(HasShape& hasShape, const Step start) : ScheduledEvent(hasShape.getSimulation(), Config::stepsToDelayBeforeTryingAgainToFollowLeader, start), m_hasShape(hasShape) { }
void CanFollowEvent::execute() { m_hasShape.m_canFollow.tryToMove(); }
void CanFollowEvent::clearReferences() { m_hasShape.m_canFollow.m_event.clearPointer(); }
