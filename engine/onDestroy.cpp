#include "onDestroy.h"
#include "actors/actors.h"
#include "area/area.h"
#include "objective.h"
#include "project.h"
#include "deserializationMemo.h"
OnDestroy::OnDestroy(const Json& data, DeserializationMemo& deserializationMemo, const ActorOrItemReference& destroyed) :
	m_destroyed(destroyed)
{
	deserializationMemo.m_onDestroys[data.get<uintptr_t>()] = this;
}
void OnDestroy::subscribe(HasOnDestroySubscriptions& hasSubscription) { m_subscriptions.insert(&hasSubscription); }
void OnDestroy::unsubscribe(HasOnDestroySubscriptions& hasSubscription) { m_subscriptions.erase(&hasSubscription); }
void OnDestroy::merge(OnDestroy& onDestroy)
{
	m_subscriptions.insert(onDestroy.m_subscriptions.begin(), onDestroy.m_subscriptions.end());
}
void OnDestroy::unsubscribeAll()
{
	auto subscriptions = m_subscriptions;
	for(HasOnDestroySubscriptions* hasSubscription : subscriptions)
		unsubscribe(*hasSubscription);
}
OnDestroy::~OnDestroy()
{
	for(HasOnDestroySubscriptions* hasSubscription : m_subscriptions)
		hasSubscription->callback(m_destroyed);
}
std::mutex HasOnDestroySubscriptions::m_mutex;
HasOnDestroySubscriptions::HasOnDestroySubscriptions(const Json& data, DeserializationMemo& deserializationMemo, Area& area) : m_callback(OnDestroyCallBack::fromJson(data["callback"], deserializationMemo, area))
{
	for(const Json& onDestroy : data["onDestroys"])
		m_onDestroys.insert(deserializationMemo.m_onDestroys.at(onDestroy.get<uintptr_t>()));
}
void HasOnDestroySubscriptions::subscribe(OnDestroy& onDestroy)
{
	assert(hasCallBack());
	assert(!m_onDestroys.contains(&onDestroy));
	m_onDestroys.insert(&onDestroy);
	onDestroy.subscribe(*this);
}
void HasOnDestroySubscriptions::subscribeThreadSafe(OnDestroy& onDestroy)
{
	std::lock_guard lock(m_mutex);
	subscribe(onDestroy);
}
void HasOnDestroySubscriptions::unsubscribe(OnDestroy& onDestroy)
{
	assert(m_onDestroys.contains(&onDestroy));
	onDestroy.unsubscribe(*this);
	m_onDestroys.erase(&onDestroy);
}
void HasOnDestroySubscriptions::maybeUnsubscribe(OnDestroy& onDestroy)
{
	if(m_onDestroys.contains(&onDestroy))
		unsubscribe(onDestroy);
}
void HasOnDestroySubscriptions::unsubscribeAll()
{
	for(OnDestroy* onDestroy : m_onDestroys)
		onDestroy->unsubscribe(*this);
	m_onDestroys.clear();
}
void HasOnDestroySubscriptions::setCallback(std::unique_ptr<OnDestroyCallBack>&& callback) { m_callback = std::move(callback); }
void HasOnDestroySubscriptions::callback(const ActorOrItemReference& destroyed)
{
	assert(!hasCallBack());
	unsubscribeAll();
	assert(m_callback != nullptr);
	auto cb = std::move(m_callback);
	m_callback = nullptr;
	cb->callback(destroyed);
}
Json HasOnDestroySubscriptions::toJson() const
{
	Json output;
	if(m_callback != nullptr)
		output["callback"] = m_callback->toJson();
	if(!m_onDestroys.empty())
		output["onDestroy"] = m_onDestroys;
	return output;
}
bool HasOnDestroySubscriptions::hasCallBack() const { return m_callback != nullptr; }
HasOnDestroySubscriptions::~HasOnDestroySubscriptions() { unsubscribeAll(); }
std::unique_ptr<OnDestroyCallBack> OnDestroyCallBack::fromJson(const Json& data, DeserializationMemo& deserializationMemo, Area& area)
{
	// TODO: This is questionable.
	if(data.contains("project"))
		return std::make_unique<ResetProjectOnDestroyCallBack>(data, deserializationMemo);
	if(data.contains("canSee"))
		return std::make_unique<OnSightOnDestroyCallBack>(data, deserializationMemo, area);
	else
	{
		assert(data.contains("objective"));
		return std::make_unique<CancelObjectiveOnDestroyCallBack>(data, deserializationMemo, area);
	}
}
// Cancel objective call back.
CancelObjectiveOnDestroyCallBack::CancelObjectiveOnDestroyCallBack(ActorReference actor, Objective& objective, Area& area) : m_objective(objective), m_area(area), m_actor(actor) { }
CancelObjectiveOnDestroyCallBack::CancelObjectiveOnDestroyCallBack(const Json& data, DeserializationMemo& deserializationMemo, Area& area) :
	m_objective(*deserializationMemo.m_objectives.at(data["objective"].get<uintptr_t>())),
	m_area(area),
	m_actor(data["actor"], area.getActors().m_referenceData) { }
void CancelObjectiveOnDestroyCallBack::callback(const ActorOrItemReference&)
{
	m_area.getActors().objective_canNotCompleteObjective(m_actor.getIndex(m_area.getActors().m_referenceData), m_objective);
}
Json CancelObjectiveOnDestroyCallBack::toJson() const
{
	return {{"actor", m_actor}, {"objective"}, &m_objective};
}
// Cancel project call back.
ResetProjectOnDestroyCallBack::ResetProjectOnDestroyCallBack(Project& project) : m_project(project) { }
ResetProjectOnDestroyCallBack::ResetProjectOnDestroyCallBack(const Json& data, DeserializationMemo& deserializationMemo) :
	m_project(*deserializationMemo.m_projects.at(data["project"].get<uintptr_t>())) { }
void ResetProjectOnDestroyCallBack::callback(const ActorOrItemReference&)
{
	m_project.reset();
}
Json ResetProjectOnDestroyCallBack::toJson() const
{
	std::unreachable();
	//return {{"project"}, m_project};
}

