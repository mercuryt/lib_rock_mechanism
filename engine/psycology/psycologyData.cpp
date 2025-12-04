#include "psycologyData.h"
#include "../config.h"
// Psycology Data
void PsycologyData::operator+=(const PsycologyData& other) { m_data += other.m_data; }
void PsycologyData::operator*=(const float& scale) { m_data = m_data.cast<float>() * scale; }
PsycologyData PsycologyData::operator-(const PsycologyData& other) { return PsycologyData::create(m_data - other.m_data); }
void PsycologyData::addTo(const PsycologyAttribute& attribute, const PsycologyWeight& value) { m_data[(uint)attribute] += value.get(); }
void PsycologyData::set(const PsycologyAttribute& attribute, const PsycologyWeight& value) { m_data[(uint)attribute] = value.get(); }
const PsycologyWeight PsycologyData::getValueFor(const PsycologyAttribute& attribute) { return {m_data[(uint)attribute]}; }
bool PsycologyData::anyAbove(const PsycologyData& other) { return (m_data > other.m_data).any(); }
bool PsycologyData::anyBelow(const PsycologyData& other) { return (m_data < other.m_data).any(); }
SmallSet<PsycologyAttribute> PsycologyData::getAbove(const PsycologyData& other)
{
	const auto result = m_data > other.m_data;
	SmallSet<PsycologyAttribute> output;
	for(uint i = 0; i < (uint)PsycologyAttribute::Null; ++i)
		if(result[i])
			output.insert((PsycologyAttribute)i);
	return output;
}
[[nodiscard]] SmallSet<PsycologyAttribute> PsycologyData::getBelow(const PsycologyData& other)
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
				ptr->action(actor, area, *this);
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
				ptr->action(actor, area, *this);
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
void Psycology::doStep(Area& area, const ActorIndex& actor)
{
	auto delta = m_current.m_data - m_baseLine.m_data;
	auto toAddToBaseLine = (delta.abs2() > Config::minimumDeltaToAlterPsycologyBaselineSquared.get());
	// Values greater then 0 become 1. Values less become -1. 0 is unchanged.
	auto deltaScaledToOne =(delta > 0).cast<int16_t>() - (delta < 0).cast<int16_t>();
	auto toAddToBaseLine = deltaScaledToOne * toAddToBaseLine.cast<int16_t>();
	m_baseLine.m_data += toAddToBaseLine;
	auto toSubtractFromCurrent = delta.abs2() > Config::maximumDeltaToAlterPsycologyCurrentSquared.get();
	m_current.m_data -= deltaScaledToOne * toSubtractFromCurrent;
}
void Psycology::addToCurrent(const PsycologyAttribute& attribute, const PsycologyWeight& value, Area& area, const ActorIndex& actor)
{
	m_current.addTo(attribute, value);
	checkThreasholds(area, actor);
}
void Psycology::addToBaseLine(const PsycologyAttribute& attribute, const PsycologyWeight& value, Area& area, const ActorIndex& actor)
{
	m_baseLine.addTo(attribute, value);
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
		PsycologyWeight newHighTrigger;
		const auto max = std::ranges::min_element(callbacks, {}, [&](const std::unique_ptr<PsycologyCallback>& ptr) { return ptr->threshold; });
		newHighTrigger = (*max)->threshold;
		m_highTriggers.set(attribute, newHighTrigger);
	}
}
void Psycology::unregisterLowCallback(const PsycologyAttribute& attribute, PsycologyCallback& callback)
{
	bool wasLowest = m_lowTriggers.getValueFor(attribute) == callback.threshold;
	auto& callbacks = m_lowCallbacks[(uint)attribute];
	callbacks.erase(std::ranges::find_if(callbacks, [&](const std::unique_ptr<PsycologyCallback>& ptr){ return &*ptr == &callback; }));
	if(wasLowest)
	{
		PsycologyWeight newLowTrigger;
		const auto max = std::ranges::min_element(callbacks, {}, [&](const std::unique_ptr<PsycologyCallback>& ptr) { return ptr->threshold; });
		newLowTrigger = (*max)->threshold;
		m_lowTriggers.set(attribute, newLowTrigger);
	}
}