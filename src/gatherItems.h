#pragma once
#include "objective.h"
#include <unordered_map>
template<class ItemType, class Item, class Actor, class Block>
class GatherItemsObjective : Objective
{
	Actor& m_actor;
	std::unordered_map<const ItemType*, uint32_t> m_items; // Store a copy, not a reference.
	Block& m_location;
	CanReserve m_canReserve;
	GatherItemsObjective(Actor& a, std::unordered_map<const ItemType*, u_int32_t>& i, Block& l) : m_actor(a), m_item(i), m_location(l) { }
	void execute()
	{
		// Check if we are dropping off an item.
		if(m_actor.m_carrying != nullptr && m_actor.m_location == m_location)
		{
			assert(m_items.contains(actor.m_carrying));
			m_items.push_back(m_actor.m_carrying);
			// Transfer reservation ownership from actor to project.
			m_actor.m_carrying.m_reservable.unreserve(m_actor.m_canReserve);
			m_actor.m_carrying.m_reservable.reserve(m_canReserve);
			m_actor.drop();
		}
		// Check if we are already in position to pickup an item.
		for(Item* item : m_actor.m_location->m_items)
			if(item.m_reservable.contains(m_actor.m_canReserve))
			{
				assert(m_items.contains(item.m_itemType));
				m_actor.pickup(item);
				m_actor.setDestination(m_location);
			}
		// Check if we can path to an item.
		for(auto& [itemType, remainingCount] : m_items)
			if(remainingCount != 0)
			{
				auto predicate = [&](Block* block) { return block->m_items.contains(itemType) && !block->m_items.at(itemType)->m_reservable.isFullyReserved()};
				std::vector<Block*> pathToItem = path::getForActorToPredicate(m_actor, predicate);
				if(pathToItem.empty())
					continue;
				Item& item = *pathToItem.back()->m_items.at(itemType);
				--remainingCount;
				item.m_reservable.reserve(m_actor.m_canReserve);
				return;
			}
		// No result found for any item.
		m_actor.cannotCompleteObjective(*this);
	}
}
