#pragma once
#include <vector>
#include <utility>
#include <memory>
#include "reference.h"
#include "deserializationMemo.h"
#include "onDestroy.h"
#include "../lib/json.hpp"

enum class OnSightCallBackType
{

};
struct OnSightCallBack
{
	// Return true from action to remove the callback after running.
	[[nodiscard]] virtual bool execute(const ActorReference& canSee, const ActorReference& canBeSeen) = 0;
	// toJson must provide an OnSightCallBackType to be used by HasOnSight json constructor to select the correct callback type to load.
	[[nodiscard]] virtual Json toJson() const = 0;
	virtual void fromJson(const Json& data, DeserializationMemo& deserializationMemo, Area& area) const = 0;
	virtual ~OnSightCallBack() = default;
};
// Contains an actor, a callback, and an OnDestroyCallBack subscription holder.
// The subscription is used to remove this from HasOnSight when the referenced actor is destoryed.
struct HasOnSightActorData
{
	std::unique_ptr<OnSightCallBack> callback;
	ActorReference actor;
	HasOnSightActorData(const ActorReference a, std::unique_ptr<OnSightCallBack>&& c);
	HasOnSightActorData(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	Json toJson() const;
};
// Contains callbacks.
class HasOnSight
{
	std::vector<HasOnSightActorData> m_actors;
	std::vector<std::pair<FactionId, std::unique_ptr<OnSightCallBack>>> m_factions;
	HasOnDestroySubscriptions m_onDestroy;
public:
	HasOnSight() = default;
	HasOnSight(const ActorReference& canSee, Area& area);
	HasOnSight(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	HasOnSight(HasOnSight&& other) noexcept = default;
	HasOnSight(const HasOnSight&) { assert(false); std::unreachable(); }
	HasOnSight& operator=(HasOnSight&& other) noexcept = default;
	HasOnSight& operator=(const HasOnSight&) { assert(false); std::unreachable(); }
	void setOwner(const ActorReference& canSee, Area& area);
	void addForActor(const ActorReference& canBeSeen, std::unique_ptr<OnSightCallBack>&& callback, Area& area);
	void addForFaction(const FactionId& faction, std::unique_ptr<OnSightCallBack>&& callback);
	void removeForActor(OnSightCallBack& callback);
	void removeForFaction(OnSightCallBack& callback);
	void removeAllForActor(const ActorReference& canBeSeen);
	void removeAllForFaction(const FactionId& faction);
	void execute(Area& area, const ActorReference& canSee, const SmallSet<ActorReference>& canNowBeSeen);
	void execute(Area& area, const ActorReference& canSee, const ActorReference& canNowBeSeen);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool empty() const;
};

// Clears Callbacks from HasOnSight which refer to actors who no longer exist.
class OnSightOnDestroyCallBack final : public OnDestroyCallBack
{
	Area& m_area;
	ActorReference m_canSee;
public:
	OnSightOnDestroyCallBack(const ActorReference& canSee, Area& area);
	OnSightOnDestroyCallBack(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void callback(const ActorOrItemReference& destroyed);
	[[nodiscard]] Json toJson() const;
};
