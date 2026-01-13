#pragma once
#include "conversation.h"
#include "../numericTypes/idTypes.h"

struct PraiseScheduledEvent;
class PraiseObjective final : public ConversationObjective
{
public:
	PraiseObjective(Area& area, const ActorIndex& receipient, const std::string subject);
	PraiseObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo);
	[[nodiscard]] std::string name() const override { return "praise"; }
	void onComplete(Area& area) override;
};