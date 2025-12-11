#include "psycologyData.h"
#include "../config.h"
#include "../objectives/flee.h"
#include "../area/area.h"
// Psycology Data
void PsycologyData::operator+=(const PsycologyData& other) { m_data += other.m_data; }
void PsycologyData::operator*=(const float& scale) { m_data = m_data.cast<float>() * scale; }
PsycologyData PsycologyData::operator-(const PsycologyData& other) const { return PsycologyData::create(m_data - other.m_data); }
void PsycologyData::addTo(const PsycologyAttribute& attribute, const PsycologyWeight& value) { m_data[(uint)attribute] += value.get(); }
void PsycologyData::set(const PsycologyAttribute& attribute, const PsycologyWeight& value) { m_data[(uint)attribute] = value.get(); }
void PsycologyData::setAllToZero() { m_data = 0; }
const PsycologyWeight PsycologyData::getValueFor(const PsycologyAttribute& attribute) const { return {m_data[(uint)attribute]}; }
bool PsycologyData::anyAbove(const PsycologyData& other) const { return (m_data > other.m_data).any(); }
bool PsycologyData::anyBelow(const PsycologyData& other) const { return (m_data < other.m_data).any(); }
SmallSet<PsycologyAttribute> PsycologyData::getAbove(const PsycologyData& other) const
{
	const auto result = m_data > other.m_data;
	SmallSet<PsycologyAttribute> output;
	for(uint i = 0; i < (uint)PsycologyAttribute::Null; ++i)
		if(result[i])
			output.insert((PsycologyAttribute)i);
	return output;
}
[[nodiscard]] SmallSet<PsycologyAttribute> PsycologyData::getBelow(const PsycologyData& other) const
{
	const auto result = m_data < other.m_data;
	SmallSet<PsycologyAttribute> output;
	for(uint i = 0; i < (uint)PsycologyAttribute::Null; ++i)
		if(result[i])
			output.insert((PsycologyAttribute)i);
	return output;
}
PsycologyData PsycologyData::create(const decltype(m_data) data)
{
	PsycologyData output;
	output.m_data = data;
	return output;
}
// Psycology
void Psycology::initalize()
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
			auto& callbacks = m_highCallbacks[(uint)attribute];
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
			auto& callbacks = m_lowCallbacks[(uint)attribute];
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
void Psycology::apply(const PsycologyEvent& event, Area& area, const ActorIndex& actor)
{
	for(uint8_t i = 0; i != (uint)PsycologyAttribute::Null; ++i)
	{
		const PsycologyAttribute attribute = (PsycologyAttribute)i;
		for(const std::unique_ptr<PsycologyCallback>& ptr : event.highCallbacks[i])
		{
			if(m_highTriggers.getValueFor(attribute) > ptr->threshold)
				m_highTriggers.set(attribute, ptr->threshold);
			m_highCallbacks[i].push_back(std::move(ptr));
		}
		for(const std::unique_ptr<PsycologyCallback>& ptr : event.lowCallbacks[i])
		{
			if(m_lowTriggers.getValueFor(attribute) > ptr->threshold)
				m_lowTriggers.set(attribute, ptr->threshold);
			m_lowCallbacks[i].push_back(std::move(ptr));
		}
	}
	m_current += event.deltas;
	checkThreasholds(area, actor);
}
void Psycology::addAll(const PsycologyData& deltas, Area& area, const ActorIndex& actor)
{
	m_current += deltas;
	checkThreasholds(area, actor);
}
void Psycology::addTo(const PsycologyAttribute& attribute, const PsycologyWeight& value, Area& area, const ActorIndex& actor)
{
	m_current.addTo(attribute, value);
	checkThreasholds(area, actor);
}
void Psycology::registerHighCallback(const PsycologyAttribute& attribute, std::unique_ptr<PsycologyCallback>&& callback)
{
	if(m_highTriggers.getValueFor(attribute) > callback->threshold)
		m_highTriggers.set(attribute, callback->threshold);
	m_highCallbacks[(uint)attribute].push_back(std::move(callback));
}
void Psycology::registerLowCallback(const PsycologyAttribute& attribute, std::unique_ptr<PsycologyCallback>&& callback)
{
	if(m_lowTriggers.getValueFor(attribute) > callback->threshold)
		m_lowTriggers.set(attribute, callback->threshold);
	m_lowCallbacks[(uint)attribute].push_back(std::move(callback));
}
void Psycology::unregisterHighCallback(const PsycologyAttribute& attribute, PsycologyCallback& callback)
{
	bool wasLowest = m_highTriggers.getValueFor(attribute) == callback.threshold;
	auto& callbacks = m_highCallbacks[(uint)attribute];
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
	auto& callbacks = m_lowCallbacks[(uint)attribute];
	callbacks.erase(std::ranges::find_if(callbacks, [&](const std::unique_ptr<PsycologyCallback>& ptr){ return &*ptr == &callback; }));
	if(wasLowest)
	{
		// Set new highest low callback for attribute.
		const auto max = std::ranges::max_element(callbacks, {}, [&](const std::unique_ptr<PsycologyCallback>& ptr) { return ptr->threshold; });
		m_lowTriggers.set(attribute, (*max)->threshold);
	}
}
void Psycology::addFriend(const ActorReference& actor) { m_friends.insert(actor); }
void Psycology::removeFriend(const ActorReference& actor) { m_frieds.remove(actor); }
void Psycology::addFamily(const ActorReference& actor, const FamilyRelationship& relationship) { m_family.insert(actor, relationship);}
PsycologyWeight Psycology::getValueFor(const PsycologyAttribute& attribute) const { return m_current.getValueFor(attribute); }