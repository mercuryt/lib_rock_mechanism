#pragma once
#include "../dataStructures/smallSet.h"
#include "../dataStructures/smallMap.h"
#include "../numericTypes/types.h"
#include "../eventSchedule.hpp"
#include "psycologyData.h"
#include "relationship.h"

class ActorIndex;
class Area;

// Something that happens when a PsycologyAttribute passes some threashold.
// Event type is stored for removing when a temporary event expires.
class Psycology;
enum class PsycologyCallbackType
{
	// No PsycologyCallbacks have been created yet. When they are add them here.
};
struct PsycologyCallback
{
	PsycologyWeight threshold;
	virtual PsycologyEventType eventType() const = 0;
	virtual void action(const ActorIndex& actor, Area& area, Psycology& psycology, const PsycologyWeight& level) = 0;
	[[nodiscard]] virtual PsycologyCallbackType getType() const = 0;
	[[nodiscard]] virtual Json toJson() const;
	[[nodiscard]] static std::unique_ptr<PsycologyCallback> load(const Json& data, Simulation& simulation);
};
// Represents an instant change in psycological state due to some event.
// Changes Psycology::m_current and optionally adds new callbacks when passed to addAll.
struct PsycologyEvent final
{
	PsycologyData deltas;
	std::array<std::vector<std::unique_ptr<PsycologyCallback>>, (int)PsycologyAttribute::Null> highCallbacks;
	std::array<std::vector<std::unique_ptr<PsycologyCallback>>, (int)PsycologyAttribute::Null> lowCallbacks;
	PsycologyEventType type;
	PsycologyEvent(const PsycologyEventType& t) : type(t) { deltas.setAllToZero(); }
	PsycologyEvent(const PsycologyEventType& t, const PsycologyData& d) : deltas(d), type(t) { }
	PsycologyEvent(const PsycologyEventType& t, const PsycologyAttribute& attribute, const PsycologyWeight& weight) :
		PsycologyEvent(t)
	{
		deltas.addTo(attribute, weight);
	}
	PsycologyEvent(PsycologyEvent&&) noexcept = default;
	PsycologyEvent(const Json& data, Simulation& simulation);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] PsycologyEvent& operator=(PsycologyEvent&&) noexcept = default;
};
class PsycologyEventExpiresScheduledEvent final : public ScheduledEvent
{
	PsycologyData m_deltas;
	ActorId m_actor;
	PsycologyEventType m_eventType;
public:
	PsycologyEventExpiresScheduledEvent(const Step& delay, const PsycologyEventType& eventType, const PsycologyData& deltas, const ActorId& actor, Simulation& simulation, const Step& start = Step::null());
	void execute(Simulation& simulation, Area* area) override;
	void clearReferences(Simulation& simulation, Area* area) override;
	[[nodiscard]] Json toJson() const;
};
// To be used by Actors
class Psycology final
{
	PsycologyData m_current;
	PsycologyData m_highTriggers;
	PsycologyData m_lowTriggers;
	std::array<std::vector<std::unique_ptr<PsycologyCallback>>, (int)PsycologyAttribute::Null> m_highCallbacks;
	std::array<std::vector<std::unique_ptr<PsycologyCallback>>, (int)PsycologyAttribute::Null> m_lowCallbacks;
	SmallSet<ActorId> m_friends;
	SmallMap<ActorId, FamilyRelationship> m_family;
	ActorHasRelationships m_relationships;
	SmallSetStable<HasScheduledEvent<PsycologyEventExpiresScheduledEvent>> m_expirationEvents;
	SmallMap<PsycologyEventType, Step> m_cooldowns;
	// Possibly trigger callbacks.
	void checkThreasholds(Area& area, const ActorIndex& actor);
	void setExpiration(const Step& duration, Simulation& simulation, const ActorId& actor, const PsycologyEventType& eventType, const PsycologyData& eventDeltas);
public:
	void initalize();
	// Record callbacks, modify current, check threasholds.
	void apply(PsycologyEvent& event, Area& area, const ActorIndex& actor, const Step& duration = Step::null(), const Step& cooldown = Step::null());
	void remove(const PsycologyEventType& eventType, const PsycologyData& deltas, Area& area, const ActorIndex& actor);
	void registerHighCallback(const PsycologyAttribute& attribute, std::unique_ptr<PsycologyCallback>&& callback);
	void registerLowCallback(const PsycologyAttribute& attribute, std::unique_ptr<PsycologyCallback>&& callback);
	void unregisterHighCallback(const PsycologyAttribute& attribute, PsycologyCallback& callback);
	void unregisterLowCallback(const PsycologyAttribute& attribute, PsycologyCallback& callback);
	void addFriend(const ActorId& actor);
	void removeFriend(const ActorId& actor);
	void addFamily(const ActorId& actor, const FamilyRelationship& relationship);
	[[nodiscard]] ActorHasRelationships& getRelationships() { return m_relationships; };
	[[nodiscard]] const ActorHasRelationships& getRelationships() const { return m_relationships; };
	[[nodiscard]] const SmallSet<ActorId>& getFriends() const { return m_friends; }
	[[nodiscard]] const SmallMap<ActorId, FamilyRelationship>& getFamily() const { return m_family; }
	[[nodiscard]] PsycologyWeight getValueFor(const PsycologyAttribute& attribute) const;
	[[nodiscard]] Json toJson() const;
	static Psycology load(const Json& data, Simulation& simulation);
	friend class PsycologyEventExpiresScheduledEvent;
};

// A pointer to a PsycologyCallback and some lifetime management methods for it.
// TODO: unused
template<typename PsycologyCallbackType, PsycologyAttribute attribute, bool high>
class PsycologyCallbackHolder final
{
	PsycologyCallback* m_callback;
public:
	template<typename ...Args>
	void create(Psycology& psycology, Args&& ...args)
	{
		std::unique_ptr callback = std::make_unique<PsycologyCallbackType>(args...);
		m_callback = callback.get();
		if constexpr (high)
			psycology.registerHighCallback(attribute, std::move(callback));
		else
			psycology.registerLowCallback(attribute, std::move(callback));
	}
	void cancel(Psycology& psycology)
	{
		if constexpr(high)
			psycology.unregisterHighCallback(attribute, *m_callback);
		else
			psycology.unregisterLowCallback(attribute, *m_callback);
		clear();
	}
	void clear()
	{
		assert(m_callback != nullptr);
		m_callback = nullptr;
	}
	[[nodiscard]] bool empty() const { return m_callback == nullptr; }
};