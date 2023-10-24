#pragma once
#include <list>
#include <functional>
#include <cassert>
#include <unordered_set>

class HasOnDestroySubscription;

class OnDestroy
{
	std::unordered_set<HasOnDestroySubscription*> m_subscriptions;
public:
	void subscribe(HasOnDestroySubscription& hasSubscription);
	void unsubscribe(HasOnDestroySubscription& hasSubscription);
	~OnDestroy();
};

class HasOnDestroySubscription
{
	std::function<void()> m_callback;
	OnDestroy* m_onDestroy;
public:
	HasOnDestroySubscription() : m_callback(nullptr), m_onDestroy(nullptr) { }
	void subscribe(OnDestroy& onDestroy, std::function<void()>& cb);
	void unsubscribe();
	void maybeUnsubscribe();
	void callback();
	~HasOnDestroySubscription();
};
