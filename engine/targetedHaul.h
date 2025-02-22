// Send specific actors to haul a specific item to a specific place.
#pragma once
#include "project.h"
#include "objective.h"
#include "reference.h"
#include "items/itemQuery.h"

struct DeserializationMemo;

class TargetedHaulProject final : public Project
{
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const { return {{ItemQuery::create(m_item), Quantity::create(1u)}}; }
	std::vector<std::pair<ActorQuery, Quantity>> getActors() const { return {}; }
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const { return {}; }
	ItemReference m_item;
	//TODO: facing.
	void onComplete() override;
	void onDelivered(const ActorOrItemIndex& delivered) override;
	// Most projects which are directly created by the user ( dig, construct ) wait a while and then retry if they fail.
	// Despite being directly created it doesn't make sense to retry targeted hauling, so instead we cancel it with a log message.
	// TODO: log message.
	void onDelay() { cancel(); }
	void offDelay() { std::unreachable(); }
	Step getDuration() const { return Config::addToStockPileDelaySteps; }
public:
	TargetedHaulProject(const FactionId& f, Area& a, const BlockIndex& l, const ItemReference& i) : Project(f, a, l, Quantity::create(4)), m_item(i) { }
	TargetedHaulProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	Json toJson() const;
};

class AreaHasTargetedHauling
{
	Area& m_area;
	std::list<TargetedHaulProject> m_projects;
public:
	AreaHasTargetedHauling(Area& a) : m_area(a) { }
	TargetedHaulProject& begin(const SmallSet<ActorIndex>& actors, const ItemIndex& item, const BlockIndex& destination);
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void cancel(TargetedHaulProject& project);
	void complete(TargetedHaulProject& project);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
};
