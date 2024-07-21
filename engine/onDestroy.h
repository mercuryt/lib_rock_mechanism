#pragma once
#include <list>
#include <functional>
#include <cassert>
#include <unordered_set>

class HasOnDestroySubscriptions;

class OnDestroy
{
	std::unordered_set<HasOnDestroySubscriptions*> m_subscriptions;
public:
	void subscribe(HasOnDestroySubscriptions& hasSubscription);
	void unsubscribe(HasOnDestroySubscriptions& hasSubscription);
	void merge(OnDestroy& onDestroy);
	void unsubscribeAll();
	[[nodiscard]] bool empty() { return m_subscriptions.empty(); }
	~OnDestroy();
	OnDestroy(OnDestroy&) = delete;
};

class HasOnDestroySubscriptions
{
	std::function<void()> m_callback;
	std::unordered_set<OnDestroy*> m_onDestroys;
public:
	HasOnDestroySubscriptions() : m_callback(nullptr) { }
	void subscribe(OnDestroy& onDestroy);
	void unsubscribe(OnDestroy& onDestroy);
	void unsubscribeAll();
	void maybeUnsubscribe(OnDestroy& onDestroy);
	void setCallback(std::function<void()>& callback);
	void callback();
	~HasOnDestroySubscriptions();
	HasOnDestroySubscriptions(HasOnDestroySubscriptions&) = delete;
};
