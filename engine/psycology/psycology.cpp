#include "psycology.h"
#include "../config/config.h"
#include "../area/area.h"
#include "../actors/actors.h"
#include "../simulation/simulation.h"
// PsycologyCallback
Json PsycologyCallback::toJson() const
{
	return {{"threshold", threshold}, {"type", getType()}};
}
// class method.
std::unique_ptr<PsycologyCallback> PsycologyCallback::load(const Json& data, Simulation&)
{
	switch(data["type"].get<PsycologyCallbackType>())
	{
		// For each callback type call a Json constructor, set it's threashold, and return it.

		default:
			std::unreachable();
	}
	std::unreachable();
}
// PsycologyEvent
PsycologyEvent::PsycologyEvent(const Json& data, Simulation& simulation)
{
	data["deltas"].get_to(deltas);
	for(size_t i = 0; i != data["highCallbacks"].size(); ++i)
	{
		auto& callbacks = highCallbacks[i];
		for(const Json& callbackData : data["highCallbacks"][i])
			callbacks.push_back(PsycologyCallback::load(callbackData, simulation));
	}
	for(size_t i = 0; i != data["lowCallbacks"].size(); ++i)
	{
		auto& callbacks = lowCallbacks[i];
		for(const Json& callbackData : data["lowCallbacks"][i])
			callbacks.push_back(PsycologyCallback::load(callbackData, simulation));
	}
	data["type"].get_to(type);
}
[[nodiscard]] Json PsycologyEvent::toJson() const
{
	Json output{{"deltas", deltas}, {"highCallbacks", Json::array()}, {"lowCallbacks", Json::array()}, {"type", type}};
	for(const auto& callbacksForAttribute : highCallbacks)
	{
		output["highCallbacks"].push_back(Json::array());
		auto& callbackData = output["highCallbacks"].back();
		for(const auto& callback : callbacksForAttribute)
			callbackData.push_back(callback->toJson());
	}
	for(const auto& callbacksForAttribute : lowCallbacks)
	{
		output["lowCallbacks"].push_back(Json::array());
		auto& callbackData = output["lowCallbacks"].back();
		for(const auto& callback : callbacksForAttribute)
			callbackData.push_back(callback->toJson());
	}
	return output;
}
// PsycologyEventExpiresScheduledEvent
PsycologyEventExpiresScheduledEvent::PsycologyEventExpiresScheduledEvent(const Step& delay, const PsycologyEventType& eventType, const PsycologyData& deltas, const ActorId& actor, Simulation& simulation, const Step& start) :
	ScheduledEvent(simulation, delay, start),
	m_deltas(deltas),
	m_actor(actor),
	m_eventType(eventType)
{ }
void PsycologyEventExpiresScheduledEvent::execute(Simulation& simulation, Area* area)
{
	auto& [actors, actor] = simulation.m_actors.getDataLocation(m_actor);
	Psycology& psycology = actors->psycology_get(actor);
	psycology.remove(m_eventType, m_deltas, *area, actor);
}
void PsycologyEventExpiresScheduledEvent::clearReferences(Simulation& simulation, Area*)
{
	auto& [actors, actor] = simulation.m_actors.getDataLocation(m_actor);
	Psycology& psycology = actors->psycology_get(actor);
	auto& events = psycology.m_expirationEvents;
	auto found = events.findIf([&](const HasScheduledEvent<PsycologyEventExpiresScheduledEvent>& holder) { return holder.getEvent() == this; });
	assert(found != events.end());
	events.erase(found);
}
Json PsycologyEventExpiresScheduledEvent::toJson() const
{
	return {{"duration", duration()}, {"start", m_startStep}, {"deltas", m_deltas}, {"type", m_eventType}, {"actor", m_actor}};
}
// Psycology
void Psycology::initialize()
{
	m_current.setAllToZero();
	m_highTriggers.setAllToZero();
	m_lowTriggers.setAllToZero();
}
void Psycology::checkThreasholds(Area& area, const ActorIndex& actor)
{
	if(m_current.anyAbove(m_highTriggers))
	{
		for(const PsycologyAttribute& attribute : m_current.getAbove(m_highTriggers))
		{
			const PsycologyWeight current = m_current.getValueFor(attribute);
			auto& callbacks = m_highCallbacks[(int)attribute];
			auto iter = std::partition(callbacks.begin(), callbacks.end(), [&](const std::unique_ptr<PsycologyCallback>& callback) { return callback->threshold <= current; });
			std::vector<std::unique_ptr<PsycologyCallback>> toCallAndDestroy;
			std::ranges::move(iter, callbacks.end(), std::back_inserter(toCallAndDestroy));
			callbacks.erase(iter, callbacks.end());
			// All triggered callbacks are removed from the list and then called.
			// They are responsible for cleaning up their own holders.
			for(const std::unique_ptr<PsycologyCallback>& ptr : toCallAndDestroy)
				ptr->action(actor, area, *this, current);
			// Update highTriggers.
			if(!callbacks.empty())
			{
				PsycologyWeight newHighTrigger;
				const auto min = std::ranges::min_element(callbacks, {}, [&](const std::unique_ptr<PsycologyCallback>& ptr) { return ptr->threshold; });
				newHighTrigger = (*min)->threshold;
				m_highTriggers.set(attribute, newHighTrigger);
			}
		}
	}
	if(m_current.anyBelow(m_lowTriggers))
	{
		for(const PsycologyAttribute& attribute : m_current.getBelow(m_lowTriggers))
		{
			const PsycologyWeight current = m_current.getValueFor(attribute);
			auto& callbacks = m_lowCallbacks[(int)attribute];
			auto iter = std::partition(callbacks.begin(), callbacks.end(), [&](const std::unique_ptr<PsycologyCallback>& callback) { return callback->threshold >= current; });
			std::vector<std::unique_ptr<PsycologyCallback>> toCallAndDestroy;
			std::ranges::move(iter, callbacks.end(), std::back_inserter(toCallAndDestroy));
			callbacks.erase(iter, callbacks.end());
			// All triggered callbacks are removed from the list and then called.
			// They are responsible for cleaning up their own holders.
			for(const std::unique_ptr<PsycologyCallback>& ptr : toCallAndDestroy)
				ptr->action(actor, area, *this, current);
			// Update lowTriggers.
			if(!callbacks.empty())
			{
				PsycologyWeight newLowTrigger;
				const auto max = std::ranges::max_element(callbacks, {}, [&](const std::unique_ptr<PsycologyCallback>& ptr) { return ptr->threshold; });
				newLowTrigger = (*max)->threshold;
				m_lowTriggers.set(attribute, newLowTrigger);
			}
		}
	}
}
void Psycology::setExpiration(const Step& duration, Simulation& simulation, const ActorId& actor, const PsycologyEventType& eventType, const PsycologyData& eventDeltas)
{
		std::unique_ptr<HasScheduledEvent<PsycologyEventExpiresScheduledEvent>> holder = std::make_unique<HasScheduledEvent<PsycologyEventExpiresScheduledEvent>>(simulation.m_eventSchedule);
		holder->schedule(duration, eventType, eventDeltas, actor, simulation);
		m_expirationEvents.insert(std::move(holder));
}
void Psycology::apply(PsycologyEvent& event, Area& area, const ActorIndex& actor, const Step& duration, const Step& cooldown)
{
	auto found = m_cooldowns.find(event.type);
	if(found != m_cooldowns.end() && found->second < area.m_simulation.m_step)
		return;
	for(int i = 0; i != (int)PsycologyAttribute::Null; ++i)
	{
		const PsycologyAttribute attribute = (PsycologyAttribute)i;
		for(std::unique_ptr<PsycologyCallback>& ptr : event.highCallbacks[i])
		{
			if(m_highTriggers.getValueFor(attribute) > ptr->threshold)
				m_highTriggers.set(attribute, ptr->threshold);
			m_highCallbacks[i].push_back(std::move(ptr));
		}
		for(std::unique_ptr<PsycologyCallback>& ptr : event.lowCallbacks[i])
		{
			if(m_lowTriggers.getValueFor(attribute) > ptr->threshold)
				m_lowTriggers.set(attribute, ptr->threshold);
			m_lowCallbacks[i].push_back(std::move(ptr));
		}
	}
	m_current += event.deltas;
	checkThreasholds(area, actor);
	if(duration.exists())
		setExpiration(duration, area.m_simulation, area.getActors().getId(actor), event.type, event.deltas);
	if(cooldown.exists())
		m_cooldowns.getOrCreate(event.type) = cooldown;
	else
		// Do not allow another event of the same same type for at least duration, we don't have the infastructure to expire them seperately.
		m_cooldowns.getOrCreate(event.type) = duration;
}
void Psycology::remove(const PsycologyEventType& eventType, const PsycologyData& deltas, Area& area, const ActorIndex& actor)
{
	SmallSet<PsycologyAttribute> attributesToUpdateThreasholds;
	for(int i = 0; i != (int)PsycologyAttribute::Null; ++i)
	{
		SmallSet<PsycologyCallback*> highCallbacksToRemove;
		SmallSet<PsycologyCallback*> lowCallbacksToRemove;
		const PsycologyAttribute attribute = (PsycologyAttribute)i;
		std::erase_if(m_highCallbacks[i], [&](const std::unique_ptr<PsycologyCallback>& ptr)
		{
			if(eventType != ptr->eventType())
				return false;
			if(m_highTriggers.getValueFor(attribute) == ptr->threshold)
				attributesToUpdateThreasholds.insert(attribute);
			return true;
		});
		std::erase_if(m_lowCallbacks[i], [&](const std::unique_ptr<PsycologyCallback>& ptr)
		{
			if(eventType != ptr->eventType())
				return false;
			if(m_lowTriggers.getValueFor(attribute) == ptr->threshold)
				attributesToUpdateThreasholds.insert(attribute);
			return true;
		});
	}
	for(const PsycologyAttribute& attribute : attributesToUpdateThreasholds)
	{
		// Find the new lowest high trigger.
		if(m_highCallbacks[(int)attribute].empty())
			m_highTriggers.m_data[(int)attribute] = FLT_MAX;
		else
		{
			const auto found = std::ranges::min_element(m_highCallbacks[(int)attribute], std::ranges::less{}, [](const std::unique_ptr<PsycologyCallback>& callback){ return callback->threshold;});
			m_highTriggers.m_data[(int)attribute] = (*found)->threshold.get();
		}
		// Find the new highest low trigger.
		if(m_lowCallbacks[(int)attribute].empty())
			m_lowTriggers.m_data[(int)attribute] = FLT_MIN;
		else
		{
			const auto found = std::ranges::min_element(m_lowCallbacks[(int)attribute], std::ranges::greater{}, [](const std::unique_ptr<PsycologyCallback>& callback){ return callback->threshold;});
			m_lowTriggers.m_data[(int)attribute] = (*found)->threshold.get();
		}
	}
	m_current -= deltas;
	checkThreasholds(area, actor);
}
void Psycology::registerHighCallback(const PsycologyAttribute& attribute, std::unique_ptr<PsycologyCallback>&& callback)
{
	if(m_highTriggers.getValueFor(attribute) > callback->threshold)
		m_highTriggers.set(attribute, callback->threshold);
	m_highCallbacks[(int)attribute].push_back(std::move(callback));
}
void Psycology::registerLowCallback(const PsycologyAttribute& attribute, std::unique_ptr<PsycologyCallback>&& callback)
{
	if(m_lowTriggers.getValueFor(attribute) > callback->threshold)
		m_lowTriggers.set(attribute, callback->threshold);
	m_lowCallbacks[(int)attribute].push_back(std::move(callback));
}
void Psycology::unregisterHighCallback(const PsycologyAttribute& attribute, PsycologyCallback& callback)
{
	bool wasLowest = m_highTriggers.getValueFor(attribute) == callback.threshold;
	auto& callbacks = m_highCallbacks[(int)attribute];
	callbacks.erase(std::ranges::find_if(callbacks, [&](const std::unique_ptr<PsycologyCallback>& ptr){ return &*ptr == &callback; }));
	if(wasLowest)
	{
		// Set new lowest high callback for attribute.
		const auto min = std::ranges::min_element(callbacks, {}, [&](const std::unique_ptr<PsycologyCallback>& ptr) { return ptr->threshold; });
		m_highTriggers.set(attribute, (*min)->threshold);
	}
}
void Psycology::unregisterLowCallback(const PsycologyAttribute& attribute, PsycologyCallback& callback)
{
	bool wasLowest = m_lowTriggers.getValueFor(attribute) == callback.threshold;
	auto& callbacks = m_lowCallbacks[(int)attribute];
	callbacks.erase(std::ranges::find_if(callbacks, [&](const std::unique_ptr<PsycologyCallback>& ptr){ return &*ptr == &callback; }));
	if(wasLowest)
	{
		// Set new highest low callback for attribute.
		const auto max = std::ranges::max_element(callbacks, {}, [&](const std::unique_ptr<PsycologyCallback>& ptr) { return ptr->threshold; });
		m_lowTriggers.set(attribute, (*max)->threshold);
	}
}
void Psycology::addFriend(const ActorId& actor) { m_friends.insert(actor); }
void Psycology::removeFriend(const ActorId& actor) { m_friends.erase(actor); }
void Psycology::addFamily(const ActorId& actor, const FamilyRelationship& relationship) { m_family.insert(actor, relationship);}
PsycologyWeight Psycology::getValueFor(const PsycologyAttribute& attribute) const { return m_current.getValueFor(attribute); }
Json Psycology::toJson() const
{
	Json output{
		{"current", m_current},
		{"highTriggers", m_highTriggers},
		{"lowTriggers", m_lowTriggers},
		{"highCallbacks", Json::array()},
		{"lowCallbacks", Json::array()},
		{"friends", m_friends},
		{"family", m_family},
		{"relationships", m_relationships},
		{"expirationEvents", Json::array()},
		{"cooldowns", m_cooldowns},
	};
	// OuterLoop: PsycololgyAttributes.
	for(size_t i = 0; i != m_highCallbacks.size(); ++i)
	{
		// InnerLoop: Callbacks.
		auto& callbacks = m_highCallbacks[i];
		for(const auto& callbackPtr : callbacks)
			output["highCallbacks"].push_back(callbackPtr->toJson());
	}
	for(size_t i = 0; i != m_lowCallbacks.size(); ++i)
	{
		// InnerLoop: Callbacks.
		auto& callbacks = m_lowCallbacks[i];
		for(const auto& callbackPtr : callbacks)
			output["lowCallbacks"].push_back(callbackPtr->toJson());
	}
	for(const HasScheduledEvent<PsycologyEventExpiresScheduledEvent>& holder : m_expirationEvents)
	{
		const PsycologyEventExpiresScheduledEvent* event = static_cast<const PsycologyEventExpiresScheduledEvent*>(holder.getEvent());
		output["expirationEvents"].push_back(event->toJson());
	}
	return output;
}
Psycology Psycology::load(const Json& data, Simulation& simulation)
{
	Psycology output;
	data["current"].get_to(output.m_current);
	data["highTriggers"].get_to(output.m_highTriggers);
	data["lowTriggers"].get_to(output.m_lowTriggers);
	for(size_t i = 0; i != data["highCallbacks"].size(); ++i)
	{
		auto& callbacks = output.m_highCallbacks[i];
		for(const Json& callbackData : data["highCallbacks"][i])
			callbacks.push_back(PsycologyCallback::load(callbackData, simulation));
	}
	for(size_t i = 0; i != data["lowCallbacks"].size(); ++i)
	{
		auto& callbacks = output.m_lowCallbacks[i];
		for(const Json& callbackData : data["lowCallbacks"][i])
			callbacks.push_back(PsycologyCallback::load(callbackData, simulation));
	}
	data["friends"].get_to(output.m_friends);
	data["family"].get_to(output.m_family);
	data["relationships"].get_to(output.m_relationships);
	for(const Json& eventData : data["expirationEvents"])
		output.setExpiration(eventData["duration"].get<Step>(), simulation, eventData["actor"].get<ActorId>(), eventData["type"].get<PsycologyEventType>(), eventData["deltas"].get<PsycologyData>());
	data["cooldowns"].get_to(output.m_cooldowns);
	return output;
}