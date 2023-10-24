#include "onDestroy.h"
void OnDestroy::subscribe(HasOnDestroySubscription& hasSubscription) { m_subscriptions.insert(&hasSubscription); }
void OnDestroy::unsubscribe(HasOnDestroySubscription& hasSubscription) { m_subscriptions.erase(&hasSubscription); }
OnDestroy::~OnDestroy()
{
	for(HasOnDestroySubscription* hasSubscription : m_subscriptions)
		hasSubscription->callback();
}

void HasOnDestroySubscription::subscribe(OnDestroy& onDestroy, std::function<void()>& cb) 
{ 
	assert(m_onDestroy == nullptr);
	assert(m_callback == nullptr);
	m_onDestroy = &onDestroy;
	m_callback = cb;
	onDestroy.subscribe(*this);
}
void HasOnDestroySubscription::unsubscribe()
{
	assert(m_onDestroy != nullptr);
	m_onDestroy->unsubscribe(*this);
	m_onDestroy = nullptr;
	m_callback = nullptr;
}
void HasOnDestroySubscription::maybeUnsubscribe()
{
	if(m_onDestroy != nullptr)
		unsubscribe();
}
void HasOnDestroySubscription::callback()
{
	auto cb = std::move(m_callback);
	m_onDestroy = nullptr;
	m_callback = nullptr;
	cb();
}
HasOnDestroySubscription::~HasOnDestroySubscription() { maybeUnsubscribe(); }
