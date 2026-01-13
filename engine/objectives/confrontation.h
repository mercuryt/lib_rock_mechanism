/*
	A social fight which starts as an argument and could esclate to insults and or brawling.
*/
#pragma once
#include "../reference.h"
#include "../objective.h"
#include "../eventSchedule.hpp"
#include "../onDestroy.h"
class ConfrontationPhaseScheduledEvent;
class ConfrontationCoolDownActorScheduledEvent;
class ConfrontationCoolDownTargetScheduledEvent;
enum class ConfrontationPhase
{
	Argument,
	Insults,
	Fighting
};
// TODO: This archeture is limited to one-on-one fights only. Generalize with something like BrawlDramaArc.
class ConfrontationObjective final : public Objective
{
	HasScheduledEvent<ConfrontationPhaseScheduledEvent> m_phaseEvent;
	HasScheduledEvent<ConfrontationCoolDownActorScheduledEvent> m_actorCoolDownEvent;
	HasScheduledEvent<ConfrontationCoolDownTargetScheduledEvent> m_targetCoolDownEvent;
	std::string m_reason;
	//TODO: callback for when ActorId is destroyed.
	ActorId m_target;
	ConfrontationPhase m_phase = ConfrontationPhase::Argument;
	bool m_targetIsWinner;
	bool m_targetHasBeenCast = false;
	bool m_fatal = false;
	bool m_violent = false;
	// ThisActor is the one with the cooldown. The other actor is the one confronting target. They may be the same actor.
	void onCoolDown(const ActorReference& actor, const ActorIndex& thisActor, Area& area);
public:
	ConfrontationObjective(const std::string& reason, const ActorId& target);
	ConfrontationObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area, const ActorIndex& actor) override;
	void cancel(Area& area, const ActorIndex& actor) override;
	void delay(Area& area, const ActorIndex& actor) override;
	void reset(Area& area, const ActorIndex& actor) override;
	void onActorCoolDown(const ActorReference& actor, Area& area);
	void onTargetCoolDown(const ActorReference& actor, Area& area);
	// Set changes to psycology and possibly spawn a new DramaArc like AccidentalHomicide or Reconciliation.
	void finalize(Area& area, const ActorIndex& actor);
	void actorGoesOffScript(Area& area, const ActorIndex& owningActor, const ActorIndex& offScriptActor) override;
	[[nodiscard]] bool targetYields(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] bool actorYields(Area& area, const ActorIndex& actor) const;
	[[nodiscard]] std::string name() const override { return "confront"; }
	friend class ConfrontationPhaseScheduledEvent;
	friend class ConfrontationCoolDownActorScheduledEvent;
	friend class ConfrontationCoolDownTargetScheduledEvent;
};

class ConfrontationPhaseScheduledEvent final : public ScheduledEvent
{
	ConfrontationObjective& m_objective;
	ActorReference m_actor;
public:
	ConfrontationPhaseScheduledEvent(const Step& duration, ConfrontationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start = Step::null());
	void execute(Simulation& simulation, Area* area) override;
	void clearReferences(Simulation& simulation, Area* area) override;
};
class ConfrontationCoolDownActorScheduledEvent final : public ScheduledEvent
{
	ConfrontationObjective& m_objective;
	ActorReference m_actor;
public:
	ConfrontationCoolDownActorScheduledEvent(const Step& duration, ConfrontationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start = Step::null());
	void execute(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class ConfrontationCoolDownTargetScheduledEvent final : public ScheduledEvent
{
	ConfrontationObjective& m_objective;
	ActorReference m_actor;
public:
	ConfrontationCoolDownTargetScheduledEvent(const Step& duration, ConfrontationObjective& objective, const ActorReference& actor, Simulation& simulation, const Step& start = Step::null());
	void execute(Simulation& simulation, Area* area) override;
	void clearReferences(Simulation& simulation, Area* area) override;
};
//TODO: add to deserialization memo.