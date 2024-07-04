#pragma once
#include "../threadedTask.hpp"
#include "../objective.h"
#include "../findsPath.h"
#include "../onDestroy.h"
#include "reservable.h"

class GiveItemObjective final : public Objective
{
	ItemIndex m_item;
	ActorIndex m_recipient;
	// This objective is dependent on receipent being alive but cannot reserve them, use onDestory instead.
	HasOnDestroySubscriptions m_hasOnDestroySubscriptions;
public:
	GiveItemObjective(Area& area, ActorIndex actor, ItemIndex item, ActorIndex recepient);
	GiveItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area) { cancel(area); }
	void reset(Area& area);
	void createOnDestroyCallbacks(Area& area);
	[[nodiscard]] std::string name() const { return "give item"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GiveItem; }
	[[nodiscard]] Json toJson() const;
};
