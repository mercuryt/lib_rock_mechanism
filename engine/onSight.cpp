#include "onSight.h"
#include "actors/actors.h"
#include "area/area.h"
HasOnSight::HasOnSight(const ActorReference& canSee, Area& area)
{
	setOwner(canSee, area);
}
HasOnSight::HasOnSight(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	m_onDestroy(data["onDestroy"], deserializationMemo, area)
{
	// TODO: Once an onSight callback type is defined this needs to be written with switch statements in the body operating on text representing an OnSightCallBackType enum.
	/*
	for(const Json& forActor : data["actors"])
		m_actors.emplace_back(forActor, deserializationMemo, area);
	for(const Json& pair : data["factions"])
	{
		m_factions.emplace_back(pair[0], std::make_unique<OnSightCallBack>());
		m_factions.back().second->fromJson(pair[1], deserializationMemo, area);
	}
		*/
}
void HasOnSight::setOwner(const ActorReference& canSee, Area& area)
{
	// When an actor referenced here is destroyed call the OnSightOnDestoryCallBack. It will use the stored canSee and area to remove the callback for the provided actor.
	m_onDestroy.setCallback(std::make_unique<OnSightOnDestroyCallBack>(canSee, area));
}
void HasOnSight::addForActor(const ActorReference& canBeSeen, std::unique_ptr<OnSightCallBack>&& callback, Area& area)
{
	Actors& actors = area.getActors();
	// Pass m_onDestoy to Actors to record the relationship with canBeSeen.
	actors.onDestroy_subscribe(canBeSeen.getIndex(actors.m_referenceData), m_onDestroy);
	// Store the callback.
	m_actors.emplace_back(canBeSeen, std::move(callback));
}
void HasOnSight::addForFaction(const FactionId& faction, std::unique_ptr<OnSightCallBack>&& callback)
{
	m_factions.emplace_back(faction, std::move(callback));
}
void HasOnSight::removeForActor(OnSightCallBack& callback)
{
	m_actors.erase(std::ranges::find_if(m_actors, [&](const HasOnSightActorData& data){ return data.callback.get() == &callback; }));
}
void HasOnSight::removeForFaction(OnSightCallBack& callback)
{
	m_factions.erase(std::ranges::find_if(m_factions, [&](const auto& pair){ return pair.second.get() == &callback; }));
}
void HasOnSight::removeAllForActor(const ActorReference& canBeSeen)
{
	std::ranges::remove_if(m_actors, [&](const HasOnSightActorData& data) { return data.actor == canBeSeen; });
}
void HasOnSight::removeAllForFaction(const FactionId& faction)
{
	std::ranges::remove_if(m_factions, [&](const auto& pair) { return pair.first == faction; });
}
void HasOnSight::execute(Area& area, const ActorReference& canSee, const ActorReference& canNowBeSeen)
{
	// TODO: optimize this.
	execute(area, canSee, SmallSet<ActorReference>({canNowBeSeen}));
}
Json HasOnSight::toJson() const
{
	Json output = {{"actors", Json::array()}, {"factions", Json::array()}, {"onDestroy", m_onDestroy.toJson()}};
	for(const HasOnSightActorData& data : m_actors)
		output["actors"].push_back(data.toJson());
	for(const auto& pair : m_factions)
		output["factions"].push_back({pair.first, pair.second->toJson()});
	return output;
}
void HasOnSight::execute(Area& area, const ActorReference& canSee, const SmallSet<ActorReference>& canNowBeSeen)
{
	assert(!canNowBeSeen.empty());
	SmallSet<OnSightCallBack*> toRemove;
	for(const HasOnSightActorData& data : m_actors)
	{
		if(canNowBeSeen.contains(data.actor))
		{
			bool remove = data.callback->execute(canSee, data.actor);
			if(remove)
				toRemove.insert(data.callback.get());
		}
	}
	for(OnSightCallBack* callback : toRemove)
		removeForActor(*callback);
	toRemove.clear();
	Actors& actors = area.getActors();
	SmallSet<FactionId> visibleFactions;
	for(const ActorReference& canBeSeen : canNowBeSeen)
		visibleFactions.maybeInsert(actors.getFaction(canBeSeen.getIndex(actors.m_referenceData)));
	for(const auto& [faction, callback] : m_factions)
	{
		for(const ActorReference& canBeSeen : canNowBeSeen)
		{
			if(actors.getFaction(canBeSeen.getIndex(actors.m_referenceData)) == faction)
			{
				bool remove = callback->execute(canSee, canBeSeen);
				if(remove)
					toRemove.insert(callback.get());
				break;
			}
		}
	}
	for(OnSightCallBack* callback : toRemove)
		removeForFaction(*callback);
}
bool HasOnSight::empty() const { return m_actors.empty() && m_factions.empty(); }
OnSightOnDestroyCallBack::OnSightOnDestroyCallBack(const ActorReference& canSee, Area& area) :
	m_area(area),
	m_canSee(canSee)
{ }
OnSightOnDestroyCallBack::OnSightOnDestroyCallBack(const Json& data, DeserializationMemo&, Area& area) :
	m_area(area),
	m_canSee(data["canSee"], area.getActors().m_referenceData)
{ }
void OnSightOnDestroyCallBack::callback(const ActorOrItemReference& destroyed)
{
	Actors& actors = m_area.getActors();
	actors.onSight_get(m_canSee.getIndex(actors.m_referenceData)).removeAllForActor(destroyed.toActorReference());
}
Json OnSightOnDestroyCallBack::toJson() const
{
	return {{"canSee", m_canSee}};
}
Json HasOnSightActorData::toJson() const
{
	return {{"actor", actor}, {"callback", callback->toJson()}};
}
HasOnSightActorData::HasOnSightActorData(const ActorReference a, std::unique_ptr<OnSightCallBack>&& c) :
	callback(std::move(c)),
	actor(a)
{}
HasOnSightActorData::HasOnSightActorData(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	actor(data["actor"], area.getActors().m_referenceData)
{
	callback->fromJson(data["callback"], deserializationMemo, area);
}