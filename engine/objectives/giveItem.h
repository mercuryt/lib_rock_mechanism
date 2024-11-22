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
	GiveItemObjective(Area& area, const ItemIndex& item, const ActorIndex& receipent);
	GiveItemObjective(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor) { cancel(area, actor); }
	void reset(Area& area, const ActorIndex& actor);
	void createOnDestroyCallbacks(Area& area, const ActorIndex& actor);
	[[nodiscard]] std::string name() const { return "give item"; }
	[[nodiscard]] Json toJson() const;
};
