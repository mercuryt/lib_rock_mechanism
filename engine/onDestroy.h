#pragma once
#include "json.h"
#include "reference.h"
#include "dataStructures/smallSet.h"
#include <list>
#include <functional>
#include <cassert>

class HasOnDestroySubscriptions;
class Objective;
struct DeserializationMemo;
class Area;
class Project;

class OnDestroyCallBack
{
public:
	OnDestroyCallBack() = default;
	virtual void callback(const ActorOrItemReference& destroyed) = 0;
	[[nodiscard]] virtual Json toJson() const = 0;
	virtual ~OnDestroyCallBack() = default;
	static std::unique_ptr<OnDestroyCallBack> fromJson(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
};
// TODO: storing area pointer in every callback is suboptimal.
// 	We can't pass them into the callback from ~OnDestroy without storing it there instead.
class CancelObjectiveOnDestroyCallBack final : public OnDestroyCallBack
{
	Objective& m_objective;
	Area& m_area;
	ActorReference m_actor;
public:
	CancelObjectiveOnDestroyCallBack(ActorReference actor, Objective& objective, Area& area);
	CancelObjectiveOnDestroyCallBack(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void callback(const ActorOrItemReference& destroyed) override;
	[[nodiscard]] Json toJson() const override;
};
class ResetProjectOnDestroyCallBack final : public OnDestroyCallBack
{
	Project& m_project;
public:
	ResetProjectOnDestroyCallBack(Project& project);
	ResetProjectOnDestroyCallBack(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(const ActorOrItemReference& destroyed) override;
	[[nodiscard]] Json toJson() const override;
};
// OnDestroy must be loaded from json before HasOnDestroySubscriptions.
class OnDestroy final
{
	SmallSet<HasOnDestroySubscriptions*> m_subscriptions;
	ActorOrItemReference m_destroyed;
public:
	OnDestroy(const Json& data, DeserializationMemo& deserializationMemo, const ActorOrItemReference& destroyed);
	OnDestroy(const ActorReference& actor) : m_destroyed(actor) { }
	OnDestroy(const ItemReference& item) : m_destroyed(item) { }
	void subscribe(HasOnDestroySubscriptions& hasSubscription);
	void unsubscribe(HasOnDestroySubscriptions& hasSubscription);
	void merge(OnDestroy& onDestroy);
	void unsubscribeAll();
	[[nodiscard]] bool empty() const { return m_subscriptions.empty(); }
	~OnDestroy();
	OnDestroy(OnDestroy&) = delete;
};
inline void to_json(Json& data, const OnDestroy* const& onDestroy) { data = reinterpret_cast<uintptr_t>(onDestroy); }
inline void to_json(Json& data, const OnDestroy& onDestroy) { data = reinterpret_cast<uintptr_t>(&onDestroy); }

class HasOnDestroySubscriptions final
{
	std::unique_ptr<OnDestroyCallBack> m_callback;
	SmallSet<OnDestroy*> m_onDestroys;
public:
	// Only one static mutex for the whole process.
	static std::mutex m_mutex;
	HasOnDestroySubscriptions(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	HasOnDestroySubscriptions() = default;
	HasOnDestroySubscriptions(HasOnDestroySubscriptions&&) noexcept = default;
	HasOnDestroySubscriptions& operator=(HasOnDestroySubscriptions&&) noexcept = default;
	void subscribe(OnDestroy& onDestroy);
	void subscribeThreadSafe(OnDestroy& onDestroy);
	void unsubscribe(OnDestroy& onDestroy);
	void unsubscribeAll();
	void maybeUnsubscribe(OnDestroy& onDestroy);
	void setCallback(std::unique_ptr<OnDestroyCallBack>&& callback);
	void callback(const ActorOrItemReference& destroyed);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool hasCallBack() const;
	~HasOnDestroySubscriptions();
	HasOnDestroySubscriptions(HasOnDestroySubscriptions&) = delete;
};
