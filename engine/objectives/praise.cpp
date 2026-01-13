#include "praise.h"
#include "../config/social.h"
#include "../actors/actors.h"
#include "../deserializationMemo.h"
#include "../area/area.h"
#include "../onDestroy.h"
#include "followScript.h"

PraiseObjective::PraiseObjective(Area& area, const ActorIndex& recipient, const std::string subject) :
	ConversationObjective(area, recipient, "praise for " + area.getActors().getName(recipient) + " regarding " + subject)
{ }

PraiseObjective::PraiseObjective(const Json& data, Area& area, const ActorIndex& actor, DeserializationMemo& deserializationMemo) :
	ConversationObjective(data, area, actor, deserializationMemo)
{ }
void PraiseObjective::onComplete(Area& area)
{
	Actors& actors = area.getActors();
	const ActorIndex& recipient = m_recipient.getIndex(actors.m_referenceData);
	actors.psycology_event(recipient, PsycologyEventType::Praised, PsycologyAttribute::Pride, Config::Social::prideToAddWhenPraised, Config::Social::praisePsycologyEventDuration);
}