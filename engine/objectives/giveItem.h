#pragma once
#include "../threadedTask.hpp"
#include "../objective.h"
#include "../findsPath.h"
#include "../onDestroy.h"
#include "reservable.h"

class Actor;
class Item;
class Block;

class GiveItemObjective final : public Objective
{
	Item& m_item;
	Actor& m_recipient;
	// This objective is dependent on receipent being alive but cannot reserve them, use onDestory instead.
	HasOnDestroySubscriptions m_hasOnDestroySubscriptions;
public:
	GiveItemObjective(Actor& actor, Item& item, Actor& recepient);
	GiveItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	void createOnDestroyCallbacks();
	[[nodiscard]] std::string name() const { return "give item"; }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::GiveItem; }
	[[nodiscard]] Json toJson() const;
};
