#include "onDestroy.h"
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
		hasSubscription->callback();
}

void HasOnDestroySubscriptions::subscribe(OnDestroy& onDestroy) 
{ 
	assert(!m_onDestroys.contains(&onDestroy));
	m_onDestroys.insert(&onDestroy);
	onDestroy.subscribe(*this);
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
void HasOnDestroySubscriptions::setCallback(std::function<void()>& callback) { m_callback = callback; }
void HasOnDestroySubscriptions::callback()
{
	unsubscribeAll();
	assert(m_callback != nullptr);
	auto cb = std::move(m_callback);
	m_callback = nullptr;
	cb();
}
HasOnDestroySubscriptions::~HasOnDestroySubscriptions() { unsubscribeAll(); }
