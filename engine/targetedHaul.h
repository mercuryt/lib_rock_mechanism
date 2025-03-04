// Send specific actors to haul a specific item to a specific place.
#pragma once
#include "project.h"
#include "objective.h"
#include "reference.h"
#include "items/itemQuery.h"

struct DeserializationMemo;

static constexpr Quantity maxWorkersForTargetedHaul = Quantity::create(2);
static constexpr Quantity haulQuantityForTargetedHaul = Quantity::create(1);

class TargetedHaulProject final : public Project
{
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const
	{
		if(m_target.isActor())
			return {};
		else
			return {{ItemQuery::create(m_target.toItemReference()), haulQuantityForTargetedHaul}};
	}
	std::vector<ActorReference> getActors() const
	{
		if(m_target.isItem())
			return {};
		else
			return {{m_target.toActorReference()}};
	}
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const { return {}; }
	ActorOrItemReference m_target;
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
	TargetedHaulProject(const FactionId& f, Area& a, const BlockIndex& l, const ActorOrItemReference& t) :
		Project(f, a, l, maxWorkersForTargetedHaul),
		m_target(t)
	{ }
	TargetedHaulProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	Json toJson() const;
};

class AreaHasTargetedHauling
{
	Area& m_area;
	std::list<TargetedHaulProject> m_projects;
public:
	AreaHasTargetedHauling(Area& a) : m_area(a) { }
	TargetedHaulProject& begin(const SmallSet<ActorIndex>& actors, const ActorOrItemIndex& target, const BlockIndex& destination);
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void cancel(TargetedHaulProject& project);
	void complete(TargetedHaulProject& project);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
};
