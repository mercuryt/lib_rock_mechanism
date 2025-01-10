#pragma once
#include "../objective.h"
#include "../pathRequest.h"
#include "../types.h"
class Area;
class DrinkEvent;
class DrinkObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Area&, const ActorIndex&) const { assert(false); }
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Area&, const ActorIndex&) const { assert(false); }
	DrinkObjectiveType() = default;
	DrinkObjectiveType(const Json&, DeserializationMemo&);
	[[nodiscard]] std::wstring name() const override { return L"drink"; }
};
class DrinkObjective final : public Objective
{
	HasScheduledEvent<DrinkEvent> m_drinkEvent;
	bool m_noDrinkFound = false;
public:
	DrinkObjective(Area& area);
	DrinkObjective(const Json& data, DeserializationMemo& deserializationMemo, Area& area, const ActorIndex& actor);
	void execute(Area& area, const ActorIndex& actor);
	void cancel(Area& area, const ActorIndex& actor);
	void delay(Area& area, const ActorIndex& actor);
	void reset(Area& area, const ActorIndex& actor);
	void makePathRequest(Area& area, const ActorIndex& actor);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::wstring name() const override { return L"drink"; }
	[[nodiscard]] bool canDrinkAt(Area& area, const BlockIndex& block, const Facing& facing, const ActorIndex& actor) const;
	[[nodiscard]] BlockIndex getAdjacentBlockToDrinkAt(Area& area, const BlockIndex& block, const Facing& facing, const ActorIndex& actor) const;
	[[nodiscard]] bool canDrinkItemAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const;
	[[nodiscard]] ItemIndex getItemToDrinkFromAt(Area& area, const BlockIndex& block, const ActorIndex& actor) const;
	[[nodiscard]] bool containsSomethingDrinkable(Area& area, const BlockIndex& block, const ActorIndex& actor) const;
	[[nodiscard]] bool isNeed() const { return true; }
	[[nodiscard]] NeedType getNeedType() const { return NeedType::drink; }
	friend class DrinkEvent;
	friend class DrinkPathRequest;
};
class DrinkPathRequest final : public PathRequestBreadthFirst
{
	DrinkObjective& m_drinkObjective;
public:
	DrinkPathRequest(Area& area, DrinkObjective& drob, const ActorIndex& actor);
	DrinkPathRequest(const Json& data, Area& area, DeserializationMemo& deserializationMemo);
	[[nodiscard]] FindPathResult readStep(Area& area, const TerrainFacade& terrainFacade, PathMemoBreadthFirst& memo) override;
	void writeStep(Area& area, FindPathResult& result) override;
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::wstring name() { return L"drink"; }
};
