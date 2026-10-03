#pragma once
#include "idTypes.h"
class ActorOrItemIndex;

class ActorOrItemId
{
	ItemIdWidth m_data;
	bool m_isActor;
public:
	void set(ActorId actor) { m_data = actor.get(); m_isActor = true;}
	void set(ItemId item) { m_data = item.get(); m_isActor = false;}
	[[nodiscard]] std::strong_ordering operator<=>(const ActorOrItemId& other) const = default;
	[[nodiscard]] bool operator==(const ActorOrItemId& other) const = default;
	[[nodiscard]] bool isActor() const { return m_isActor; }
	[[nodiscard]] bool isEmpty() const { return m_data == ItemId::null().get(); }
	[[nodiscard]] ActorId getActor() const { assert(m_isActor); return {m_data}; }
	[[nodiscard]] ItemId getItem() const { assert(!m_isActor); return {m_data}; }
	[[nodiscard]] ActorOrItemIndex getIndex(const Simulation& simulation) const;
	[[nodiscard]] ActorOrItemId static create(ActorId actor) { ActorOrItemId output; output.set(actor); return output; }
	[[nodiscard]] ActorOrItemId static create(ItemId item) { ActorOrItemId output; output.set(item); return output; }
	[[nodiscard]] ActorOrItemId static null() { return ActorOrItemId::create(ItemId::null());}
};