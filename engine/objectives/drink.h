#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
class Area;
class DrinkEvent;

class DrinkObjective final : public Objective
{
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	bool m_noDrinkFound = false;
public:
	DrinkObjective(Area& area);
	DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
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
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Drink; }
	[[nodiscard]] bool isNeed() const { return true; }
	friend class DrinkEvent;
	friend class DrinkPathRequest;
};
class DrinkPathRequest final : public PathRequest
{
	DrinkObjective& m_drinkObjective;
	bool m_noDrinkFound = false;
public:
	DrinkPathRequest(Area& area, DrinkObjective& drob, ActorIndex actor);
	void callback(Area& area, FindPathResult& result);
};
