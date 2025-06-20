#include "leadAndFollow.h"
#include "area/area.h"
#include "config.h"
#include "deserializationMemo.h"
#include "simulation/simulation.h"
#include "actorOrItemIndex.h"
#include "blocks/blocks.h"
#include "actors/actors.h"
#include "items/items.h"
#include "numericTypes/types.h"
void CanLead::load(const Json& data, Area& area, DeserializationMemo& deserializationMemo)
{
	m_canFollow = data.contains("isLeadingItem") ?
		&area.getItems().m_canFollow.at(data["isLeadingItem"].get<ItemIndex>()):
		&area.getActors().getCanFollow(data["isLeadingActor"].get<ActorIndex>());
	if(data.contains("locationQueue"))
		for(const Json& block: data["locationQueue"])
			m_locationQueue.push_back(block.get<BlockIndex>());
}
Json CanLead::toJson() const
{
	Json data;
	if(m_canFollow != nullptr)
	{
		if(m_canFollow->m_actorOrItemIndex.isItem())
			data["canFollowItem"] = m_canFollow->m_actorOrItemIndex.get();
		else
		{
			assert(m_canFollow->m_actorOrItemIndex.isActor());
			data["canFollowActor"] = m_canFollow->m_actorOrItemIndex.get();
		}
	}
	if(!m_locationQueue.empty())
		data["locationQueue"] = m_locationQueue;
	return data;
}
bool CanLead::canMove(Area& area)
{
	assert(m_actorOrItemIndex.getCanFollow(area) == nullptr);
	auto occupiedSet = getOccuiped(area);
	std::vector<BlockIndex> occupied(occupiedSet.begin(), occupiedSet.end());
	auto path = getLocationQueue(area);
	auto pathIter = path.begin();
	ActorOrItemIndex follower = m_canFollow->m_actorOrItemIndex;
	while(true)
	{
		if(!area.getBlocks().shape_shapeAndMoveTypeCanEnterEverOrCurrentlyWithFacing(*pathIter, follower.getShape(area), follower.getMoveType(area), follower.getFacing(area), occupied))
			return false;
		CanLead* canLead = follower.getCanLead(area);
		if(canLead == nullptr)
			return true;
		follower = canLead->m_canFollow->m_actorOrItemIndex;
	}
}
bool CanLead::isLeading() const { return m_canFollow != nullptr; }
bool CanLead::isLeadingActor(ActorIndex index) const
{
	return m_canFollow != nullptr && m_canFollow->m_actorOrItemIndex.isActor() && m_canFollow->m_actorOrItemIndex.get() == index;
}
bool CanLead::isLeadingItem(ItemIndex index) const
{
	return m_canFollow != nullptr && m_canFollow->m_actorOrItemIndex.isItem() && m_canFollow->m_actorOrItemIndex.get() == index;
}
bool CanLead::isLeadingPolymorphic(ActorOrItemIndex index) const
{
	return m_canFollow != nullptr && m_canFollow->m_actorOrItemIndex == index;
}
ActorOrItemIndex CanLead::getFollower() { return m_canFollow->m_actorOrItemIndex; }
const ActorOrItemIndex& CanLead::getFollower() const { return m_canFollow->m_actorOrItemIndex; }
// Class method.
Speed CanLead::getMoveSpeedForGroupWithAddedMass(const Area& area, std::vector<ActorOrItemIndex>& actorsAndItems, Mass addedRollingMass, Mass addedDeadMass)
{
	Mass rollingMass = addedRollingMass;
	Mass deadMass = addedDeadMass;
	Mass carryMass = 0;
	Speed lowestMoveSpeed = 0;
	for(ActorOrItemIndex index : actorsAndItems)
	{
		if(index.isItem())
		{
			ItemIndex itemIndex = index.get();
			Mass mass = area.getItems().getMass(itemIndex);
			static const MoveType& roll = MoveType::byName("roll");
			if(area.getItems().getMoveType(itemIndex) == roll)
				rollingMass += mass;
			else
				deadMass += mass;
		}
		else
		{
			assert(index.isActor());
			ActorIndex actorIndex = index.get();
			const Actors& actors = area.getActors();
			if(actors.move_canMove(actorIndex))
			{
				carryMass += actors.getUnencomberedCarryMass(actorIndex);
				Speed moveSpeed = actors.move_getSpeed(actorIndex);
				lowestMoveSpeed = lowestMoveSpeed == 0 ? moveSpeed : std::min(lowestMoveSpeed, moveSpeed);
			}
			else
				deadMass += actors.getMass(actorIndex);
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
Speed CanLead::getMoveSpeed(Area& area) const
{
	assert(m_actorOrItemIndex.isActor());
	ActorIndex actorIndex = m_actorOrItemIndex.get();
	assert(!area.getActors().isFollowing(actorIndex));
	assert(area.getActors().isLeading(actorIndex));
	ActorOrItemIndex index = m_actorOrItemIndex;
	std::vector<ActorOrItemIndex> actorsAndItems;
	while(true)
	{
		actorsAndItems.push_back(index);
		CanLead& canLead = *index.getCanLead(area);
		if(!canLead.isLeading())
			break;
		index = canLead.getFollower();
	}
	return getMoveSpeedForGroupWithAddedMass(area, actorsAndItems, 0);
}
std::deque<BlockIndex>& CanLead::getLocationQueue(Area& area)
{
	return m_actorOrItemIndex.getCanFollow(area)->getLineLeader(area).getCanLead(area)->m_locationQueue;
}
std::unordered_set<BlockIndex> CanLead::getOccuiped(Area& area)
{
	std::unordered_set<BlockIndex> output;
	ActorOrItemIndex index = m_actorOrItemIndex;
	while(true)
	{
		const auto& blocks = index.getBlocks(area);
		output.insert(blocks.begin(), blocks.end());
		CanLead& canLead = *index.getCanLead(area);
		if(!canLead.isLeading())
			break;
		index = canLead.getFollower();
	}
	return output;
}
CanFollow::CanFollow(ActorOrItemIndex a) : m_actorOrItemIndex(a) { }
void CanFollow::load(const Json& data, DeserializationMemo& deserializationMemo)
{
	if(data.contains("canLeadItem"))
		m_canLead = &deserializationMemo.itemReference(data["canLeadItem"]).m_canLead;
	else if(data.contains("canLeadActor"))
		m_canLead = &deserializationMemo.itemReference(data["canLeadActor"]).m_canLead;
	if(data.contains("eventStart"))
		m_event.schedule(m_index, data["eventStart"].get<Step>());
}
Json CanFollow::toJson() const
{
	Json data;
	if(m_canLead)
	{
		if(m_canLead->m_index.isItem())
			data["canLeadItem"] = static_cast<Item&>(m_canLead->m_index).m_id;
		else
		{
			assert(m_canLead->m_index.isActor());
			data["canLeadActor"] = static_cast<Actor&>(m_canLead->m_index).m_id;
		}
	}
	if(m_event.exists())
		data["eventStart"] = m_event.getStartStep();
	return data;
}
void CanFollow::follow(Area& area, CanLead& canLead, bool doAdjacentCheck)
{
	assert(m_canLead == nullptr);
	assert(canLead.m_canFollow == nullptr);
	if(doAdjacentCheck)
		assert(m_actorOrItemIndex.isAdjacent(area, canLead.m_actorOrItemIndex));
	canLead.m_canFollow = this;
	m_canLead = &canLead;
	std::deque<BlockIndex>& locationQueue = canLead.getLocationQueue(area);
	if(locationQueue.empty())
	{
		assert(!canLead.m_actorOrItemIndex.getCanFollow(area)->isFollowing());
		locationQueue.push_back(canLead.m_actorOrItemIndex.getLocation(area));
	}
	locationQueue.push_back(m_actorOrItemIndex.getLocation(area));
}
void CanFollow::unfollow(Area& area)
{
	assert(m_canLead != nullptr);
	m_canLead->m_canFollow = nullptr;
	m_canLead->m_locationQueue.clear();
	m_canLead = nullptr;
	if(m_actorOrItemIndex.isItem())
		area.getItems().setStatic(m_actorOrItemIndex.get(), true);
}
void CanFollow::unfollowIfFollowing(Area& area)
{
	if(m_canLead != nullptr)
		unfollow(area);
}
void CanFollow::maybeDisband(Area& area)
{
	if(isFollowing() || m_actorOrItemIndex.getCanLead(area)->isLeading())
		disband(area);
}
void CanFollow::disband(Area& area)
{
	ActorOrItemIndex leader = getLineLeader(area);
	leader.getCanLead(area)->m_locationQueue.clear();
	while(leader.getCanLead(area)->isLeading())
	{
		leader = leader.getCanLead(area)->m_canFollow->m_actorOrItemIndex;
		assert(leader.getCanLead(area)->m_locationQueue.empty());
		leader.getCanFollow(area)->unfollow(area);
	}
}
ActorOrItemIndex CanFollow::getLineLeader(Area& area)
{
	if(!isFollowing())
		return m_actorOrItemIndex;
	else
		return m_canLead->m_actorOrItemIndex.getCanFollow(area)->getLineLeader(area);
}
