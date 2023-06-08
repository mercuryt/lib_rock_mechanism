/*
 * Pub / Sub system components. Subscription classes are expecetd to call SubscribesToEvent::cancel in their destructors.
 */
#pragma once

#include <unordered_multimap>
enum class PublishedEventType { Move, Die };

class EventPublisher;

class EventSubscription
{
public:
	PublishedEventType& m_publishedEventType;
	EventPublisher* m_eventPublisher;
	EventSubscription(PublishedEventType& pet) : m_publishedEventType(pet) {}
	virtual void execute() = 0;
	void cancel();
};
class EventPublisher
{
	std::unordered_multimap<PublishedEventTypes, std::unique_ptr<EventSubscription>> m_subcriptions;
public:
	void subscribe(std::unique_ptr<EventSubscription> subscription)
	{
		subscription.m_eventPublisher = this;
		m_subcriptions[subscription.m_publishedEventType] = std::move(subscription);
	}
	//TODO: Make variadic template and pass paramaters to subscription.
	void publish(const PublishedEventType publishedEventType)
	{
		for(auto& pair : m_subcriptions)
			pair.second();
	}
	void unsubscribe(EventSubscription& eventSubscription)
	{
		//TODO: Use multimap::find.
		std::erase_if(m_subcriptions, [&](const auto& pair){ return pair.second.get() == &eventSubscription; });
	}
};
EventSubscription::cancel() { m_eventPublisher->unsubscribe(*this); }
template<class EventType>
class SubscribesToEvent
{
	EventSubscription* m_event;
	EventPublisher* m_eventPublisher;

	~SubscribesToEvent()
	{
		if(m_event != nullptr)
			unsubscribe();
	}
public:
	template<typename ...Args>
	void subscribe(Args ...args)
	{
		assert(m_event == nulptr);
		std::unique_ptr<EventSubscription> event = std::make_unique<EventType>(args...);
		m_event = event.get();
		m_eventPublisher->subscribe(std::move(event));
	}
	void unsubscribe()
	{
		assert(m_event != nullptr);
		m_eventPublisher->unsubscribe(*m_event);
	}
	void clearPointer()
	{
		assert(m_event != nullptr);
		m_event = nullptr;
	}
	void setEventPublisher(EventPublisher& eventPublisher)
	{
		assert(m_event == nulptr; );
		m_eventPublisher = &eventPublisher;
	}
};
