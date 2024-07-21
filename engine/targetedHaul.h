// Send specific actors to haul a specific item to a specific place.
#pragma once
#include "project.h"
#include "objective.h"
#include "reference.h"
#include "items/itemQuery.h"

struct DeserializationMemo;

class TargetedHaulProject final : public Project
{
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const { ItemQuery query(m_item); return {{query, 1u}}; }
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const { return {}; }
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const { return {}; }
	ItemReference m_item;
	//TODO: facing.
	void onComplete();
	void onDelivered(ActorOrItemIndex delivered);
	// Most projects which are directly created by the user ( dig, construct ) wait a while and then retry if they fail.
	// Despite being directly created it doesn't make sense to retry targeted hauling, so instead we cancel it with a log message.
	// TODO: log message.
	void onDelay() { cancel(); }
	void offDelay() { assert(false); }
	Step getDuration() const { return Config::addToStockPileDelaySteps; }
public:
	TargetedHaulProject(FactionId f, Area& a, BlockIndex l, ItemReference i) : Project(f, a, l, 4), m_item(i) { }
	TargetedHaulProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	Json toJson() const;
};

class AreaHasTargetedHauling
{
	Area& m_area;
	std::list<TargetedHaulProject> m_projects;
public:
	AreaHasTargetedHauling(Area& a) : m_area(a) { }
	TargetedHaulProject& begin(std::vector<ActorIndex> actors, ItemIndex item, BlockIndex destination);
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	void cancel(TargetedHaulProject& project);
	void complete(TargetedHaulProject& project);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
};
