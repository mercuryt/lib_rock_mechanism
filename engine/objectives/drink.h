#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
class Area;
class DrinkEvent;
class DrinkObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, ActorIndex) const { assert(false); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, ActorIndex) const { assert(false); }
	DrinkObjectiveType() = default;
	DrinkObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::string name() const { return "drink"; }
};
class DrinkObjective final : public Objective
{
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	bool m_noDrinkFound = false;
public:
	DrinkObjective(Area& area);
	DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, ActorIndex actor);
	void execute(Area& area, ActorIndex actor);
	void cancel(Area& area, ActorIndex actor);
	void delay(Area& area, ActorIndex actor);
	void reset(Area& area, ActorIndex actor);
	void makePathRequest(Area& area, ActorIndex actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() const { return "drink"; }
	[[nodiscard]] bool canDrinkAt(Area& area, const BlockIndex block, Facing facing, ActorIndex actor) const;
	[[nodiscard]] BlockIndex getAdjacentBlockToDrinkAt(Area& area, BlockIndex block, Facing facing, ActorIndex actor) const;
	[[nodiscard]] bool canDrinkItemAt(Area& area, BlockIndex block, ActorIndex actor) const;
	[[nodiscard]] ItemIndex getItemToDrinkFromAt(Area& area, BlockIndex block, ActorIndex actor) const;
	[[nodiscard]] bool containsSomethingDrinkable(Area& area, BlockIndex block, ActorIndex actor) const;
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::drink; }
	friend class DrinkEvent;
	friend class DrinkPathRequest;
};
class DrinkPathRequest final : public PathRequest
{
	DrinkObjective& m_drinkObjective;
	bool m_noDrinkFound = false;
public:
	DrinkPathRequest(Area& area, DrinkObjective& drob, ActorIndex actor);
	DrinkPathRequest(const Json& data, DeserializationMemo& deserializationMemo);
	void callback(Area& area, FindPathResult& result);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::string name() { return "drink"; }
};
