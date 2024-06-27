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
	DrinkObjective(Area& area, ActorIndex a);
	DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute(Area& area);
	void cancel(Area& area);
	void delay(Area& area);
	void reset(Area& area);
	void makePathRequest(Area& area);
	[[nodiscard]] std::string name() const { return "drink"; }
	[[nodiscard]] bool canDrinkAt(Area& area, const BlockIndex block, Facing facing) const;
	[[nodiscard]] BlockIndex getAdjacentBlockToDrinkAt(Area& area, BlockIndex block, Facing facing) const;
	[[nodiscard]] bool canDrinkItemAt(Area& area, BlockIndex block) const;
	[[nodiscard]] ItemIndex getItemToDrinkFromAt(Area& area, BlockIndex block) const;
	[[nodiscard]] bool containsSomethingDrinkable(Area& area, BlockIndex block) const;
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
	DrinkPathRequest(Area& area, DrinkObjective& drob);
	void callback(Area& area, FindPathResult result);
};
