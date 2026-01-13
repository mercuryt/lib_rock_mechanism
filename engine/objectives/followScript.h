/*
	An objective to use when an actor is following a scripted scene, such as another actor's objective or a DramaArc.
*/
#pragma once
#include "../objective.h"
#include "../numericTypes/idTypes.h"
#include "../reference.h"

class FollowScriptSubObjective final : public Objective
{
	Objective* m_parent;
	ActorReference m_parentOwner;
public:
	FollowScriptSubObjective(Objective& parent, const ActorReference& parentOwner);
	FollowScriptSubObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	// Execute does nothing, the Actor is a puppet.
	void execute(Area&, const ActorIndex&) override { }
	// Notify which ever entity created this objective that it has been canceled.
	void cancel(Area& area, const ActorIndex& actor) override;
	void delay(Area& area, const ActorIndex& actor) override;
	void reset(Area& area, const ActorIndex& actor) override;
	[[nodiscard]] std::string name() const override { return "follow script"; }
	[[nodiscard]] Json toJson() const override;
};
//TODO: add to deserialization memo.
