#pragma once

#include "cuboid.h"
#include "input.h"
#include "objective.h"
#include "project.h"
#include <memory>

class WoodCuttingProject;
class WoodCuttingThreadedTask;
// InputAction
class DesignateWoodCuttingInputAction final : public InputAction
{
	Cuboid m_blocks;
public:
	DesignateWoodCuttingInputAction(InputQueue& inputQueue, Cuboid& blocks) : InputAction(inputQueue), m_blocks(blocks) { }
	void execute();
};
class UndesignateWoodCuttingInputAction final : public InputAction
{
	Cuboid m_blocks;
public:
	UndesignateWoodCuttingInputAction(InputQueue& inputQueue, Cuboid& blocks) : InputAction(inputQueue), m_blocks(blocks) { }
	void execute();
};
// ObjectiveType
class WoodCuttingObjectiveType final : public ObjectiveType
{
public:
	[[nodiscard]] bool canBeAssigned(Actor& actor) const;
	[[nodiscard]] std::unique_ptr<Objective> makeFor(Actor& actor) const;
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::WoodCutting; }
};
// Objective
class WoodCuttingObjective final : public Objective
{
	HasThreadedTask<WoodCuttingThreadedTask> m_woodCuttingThreadedTask;
	WoodCuttingProject* m_project;
public:
	WoodCuttingObjective(Actor& a);
	WoodCuttingObjective(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	void execute();
	void cancel();
	void reset();
	void delay() { cancel(); }
	[[nodiscard]] ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::WoodCutting; }
	[[nodiscard]] std::string name() const { return "woodcutting"; }
	void joinProject(WoodCuttingProject& project);
	WoodCuttingProject* getJoinableProjectAt(const Block& block);
	friend class WoodCuttingThreadedTask;
	friend class WoodCuttingProject;
};
// Find a place to woodCutting.
class WoodCuttingThreadedTask final : public ThreadedTask
{
	WoodCuttingObjective& m_woodCuttingObjective;
	// Result is the block which will be the actors location while doing the woodCuttingging.
	FindsPath m_findsPath;
public:
	WoodCuttingThreadedTask(WoodCuttingObjective& woodCuttingObjective);
	void readStep();
	void writeStep();
	void clearReferences();
};
class WoodCuttingProject final : public Project
{
	void onDelay();
	void offDelay();
	void onComplete();
	void onCancel();
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	static uint32_t getWorkerWoodCuttingScore(Actor& actor);
	// What would the total delay time be if we started from scratch now with current workers?
public:
	// BlockFeatureType can be null, meaning the block is to be fully excavated.
	WoodCuttingProject(const Faction* faction, Block& block, std::unique_ptr<DishonorCallback> locationDishonorCallback) : 
		Project(faction, block, Config::maxNumberOfWorkersForWoodCuttingProject, std::move(locationDishonorCallback)) { }
	WoodCuttingProject(const Json& data, DeserializationMemo& deserializationMemo);
	// No toJson is needed here.
	Step getDuration() const;
	friend class HasWoodCuttingDesignationsForFaction;
};
struct WoodCuttingLocationDishonorCallback final : public DishonorCallback
{
	const Faction& m_faction;
	Block& m_location;
	WoodCuttingLocationDishonorCallback(const Faction& f, Block& l) : m_faction(f), m_location(l) { }
	WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(uint32_t oldCount, uint32_t newCount);
};
// Part of HasWoodCuttingDesignations.
class HasWoodCuttingDesignationsForFaction final
{
	const Faction& m_faction;
	std::unordered_map<Block*, WoodCuttingProject> m_data;
public:
	HasWoodCuttingDesignationsForFaction(const Faction& p) : m_faction(p) { }
	HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const Faction& faction);
	Json toJson() const;
	void designate(Block& block);
	void undesignate(Block& block);
	// To be called by undesignate as well as by WoodCuttingProject::onCancel.
	void remove(Block& block);
	void removeIfExists(Block& block);
	bool empty() const;
	friend class HasWoodCuttingDesignations;
};
// To be used by Area.
class HasWoodCuttingDesignations final
{
	std::unordered_map<const Faction*, HasWoodCuttingDesignationsForFaction> m_data;
public:
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(const Faction& faction);
	void removeFaction(const Faction& faction);
	// If blockFeatureType is null then woodCutting out fully rather then woodCuttingging out a feature.
	void designate(const Faction& faction, Block& block);
	void undesignate(const Faction& faction, Block& block);
	void remove(const Faction& faction, Block& block);
	void clearAll(Block& block);
	bool areThereAnyForFaction(const Faction& faction) const;
	bool contains(const Faction& faction, const Block& block) const { return m_data.at(&faction).m_data.contains(const_cast<Block*>(&block)); }
	WoodCuttingProject& at(const Faction& faction, const Block& block);
};
