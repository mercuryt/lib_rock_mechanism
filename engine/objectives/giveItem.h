#pragma once
#include "../objective.h"
#include "../onDestroy.h"

class GiveItemObjective final : public Objective
{
	ItemReference m_item;
	ActorReference m_recipient;
	// This objective is dependent on receipent being alive but cannot reserve them, use onDestory instead.
	HasOnDestroySubscriptions m_hasOnDestroySubscriptions;
public:
	GiveItemObjective(Area& area, ItemIndex item, ActorIndex receipent);
	GiveItemObjective(const Json& data, Area& area);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor) { cancel(area, actor); }
	void reset(Area& area, ActorIndex actor);
	void createOnDestroyCallbacks(Area& area, ActorIndex actor);
	[[nodiscard]] std::string name() const { return "give item"; }
	[[nodiscard]] Json toJson() const;
};
