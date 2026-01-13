#pragma once
//TODO: Move implimentations into cpp file and replace includes with forward declarations it imporove compile time.
#include "../geometry/cuboid.h"
#include "../eventSchedule.h"
#include "../reference.h"
#include "../reservable.h"
#include "../project.h"
#include "../numericTypes/types.h"
#include "../dataStructures/smallSet.h"

#include <memory>
#include <sys/types.h>
#include <vector>
#include <utility>
#include <tuple>
#include <list>

class StockPilePathRequest;
class StockPileProject;
class Simulation;
class ReenableStockPileScheduledEvent;
class StockPile;
class PointIsPartOfStockPiles;
struct DeserializationMemo;
struct MaterialType;
struct FindPathResult;
class Area;
class StockPileObjective;

class StockPileProject final : public Project
{
	ItemReference m_item;
	// Needed for generic items where the original item may no longer exist.
	Quantity m_quantity = Quantity::create(0);
	ItemTypeId m_itemType;
	MaterialTypeId m_solid;
	StockPile& m_stockpile;
	void onComplete() override;
	void onReserve() override;
	// Mark dispatched as true and dismiss any unassigned workers and candidates.
	void onCancel() override;
	// TODO: geometric progresson of disable duration.
	void onDelay() override;
	void offDelay() override { std::unreachable(); }
	void onPickUpRequired(const ActorOrItemIndex& required) override;
	void updateRequiredGenericReference(const ItemReference& newRef) override;
	[[nodiscard]] bool canReset() const { return false; }
	[[nodiscard]] Step getDuration() const { return Config::addToStockPileDelaySteps; }
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const;
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const;
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const;
	[[nodiscard]] SkillTypeId getSkill() const { return SkillTypeId::null(); }
	[[nodiscard]] std::string description() const;
	std::vector<ActorReference> getActors() const;
public:
	StockPileProject(const FactionId& faction, Area& area, const Point3D& point, const ItemIndex& item, const Quantity& quantity, const Quantity& maxWorkers);
	StockPileProject(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool canAddWorker(const ActorIndex& actor) const;
	// Don't recruit more workers then are needed for hauling.
	[[nodiscard]] bool canRecruitHaulingWorkersOnly() const { return true; }
	friend class AreaHasStockPilesForFaction;
	// For testing.
	[[nodiscard, maybe_unused]] ItemReference getItem() { return m_item; }
	StockPileProject(const StockPileProject&) = delete;
	StockPileProject(StockPileProject&&) noexcept = delete;
	void operator=(const StockPileProject&) noexcept = delete;
};