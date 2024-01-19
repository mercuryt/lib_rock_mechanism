#pragma once
#include "project.h"
#include "actor.h"
#include "item.h"
#include <vector>
#include <pair>
#include <tuple>
class YokeProject
{
	Item& m_toYokeTo;
	ActorQuery& m_actorQuery;
	// TODO: Yoke team.
	Actor* m_toBeYoked;
public:
	YokeProject(Item& tyt, ActorQuery& aq) : m_toYokeTo(tyt), m_actorQuery(aq), m_toBeYoked(nullptr) { }
	// TODO: Animal handling skill.
	uint32_t getDelay() const { return Config::yokeDelaySteps; }
	void onComplete()
	{
		m_toYokeTo.m_canFollow.follow(m_toBeYoked);
	}
	std::vector<std::pair<ItemQuery, uint32_t> getConsumed() { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t> getUnconsumed() { return {}; }
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() { return {}; }
	std::vector<std::pair<ActorQuery, uint32_t> getActors(){ return {{{m_toBeYoked}, 1}}; }
};
class LoadItemsProject
{
	Item& m_toLoadInto;
	Item& m_toLoad;
	uint32_t quantity;
public:
	// TODO: Strength.
	uint32_t getDelay() const { return Config::loadDelaySteps; }
	void onComplete()
	{
		m_toLoad.m_locaton->m_hasItems.transferTo(m_toLoadInto.m_hasItems);
	}
	std::vector<std::pair<ItemQuery, uint32_t> getConsumed() { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t> getUnconsumed() { return {}; }
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() { return {}; }
	std::vector<std::pair<ActorQuery, uint32_t> getActors(){ return {}; }

}
