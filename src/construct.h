#pragma once

#include "config.h"
#include "deserializationMemo.h"
#include "objective.h"
#include "reservable.h"
#include "threadedTask.h"
#include "eventSchedule.h"
#include "project.h"
#include "findsPath.h"

#include <unordered_map>
#include <vector>

class ConstructThreadedTask;
class Block;
class Faction;
struct BlockFeatureType;
class ConstructObjective;
class ConstructProject;
class HasConstructionDesignationsForFaction;
class ConstructObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Construct; }
	ConstructObjectiveType() = default;
	ConstructObjectiveType([[maybe_unused]] const Json& data, [[maybe_unused]] DeserializationMemo& deserializationMemo){ }
};
class ConstructObjective final : public Objective
{
	HasThreadedTask<ConstructThreadedTask> m_constructThreadedTask;
	Project* m_project;
public:
	ConstructObjective(Actor& a) : Objective(a, Config::constructObjectivePriority), m_constructThreadedTask(a.getThreadedTaskEngine()), m_project(nullptr) { }
	ConstructObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute();
	void cancel();
	void delay() { cancel(); }
	void reset();
	void joinProject(ConstructProject& project);
	std::string name() const { return "construct"; }
	ConstructProject* getProjectWhichActorCanJoinAdjacentTo(const Block& location, Facing facing);
	ConstructProject* getProjectWhichActorCanJoinAt(Block& block);
	bool joinableProjectExistsAt(const Block& block) const;
	bool canJoinProjectAdjacentToLocationAndFacing(const Block& block, Facing facing) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::Construct; }
	friend class ConstructThreadedTask;
	friend class ConstructProject;
};
class ConstructThreadedTask final : public ThreadedTask
{
	ConstructObjective& m_constructObjective;
	FindsPath m_findsPath;
public:
	ConstructThreadedTask(ConstructObjective& co);
	void readStep();
	void writeStep();
	void clearReferences();
};
class ConstructProject final : public Project
{
	const BlockFeatureType* m_blockFeatureType;
	const MaterialType& m_materialType;
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const;
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const;
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const;
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const;
	uint32_t getWorkerConstructScore(Actor& actor) const;
	Step getDuration() const;
	void onComplete();
	void onCancel();
	void onDelay();
	void offDelay();
public:
	// BlockFeatureType can be null, meaning the block is to be filled with a constructed wall.
	ConstructProject(const Faction* faction, Block& b, const BlockFeatureType* bft, const MaterialType& mt, std::unique_ptr<DishonorCallback> dishonorCallback) : Project(faction, b, Config::maxNumberOfWorkersForConstructionProject, std::move(dishonorCallback)), m_blockFeatureType(bft), m_materialType(mt) { }
	ConstructProject(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	// What would the total delay time be if we started from scratch now with current workers?
	friend class HasConstructionDesignationsForFaction;
};
struct ConstructionLocationDishonorCallback final : public DishonorCallback
{
	const Faction& m_faction;
	Block& m_location;
	ConstructionLocationDishonorCallback(const Faction& f, Block& l) : m_faction(f), m_location(l) { }
	ConstructionLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute([[maybe_unused]] uint32_t oldCount, [[maybe_unused]] uint32_t newCount);
};
class HasConstructionDesignationsForFaction final
{
	const Faction& m_faction;
	//TODO: More then one construct project targeting a given block should be able to exist simultaniously.
	std::unordered_map<Block*, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(const Faction& p) : m_faction(p) { }
	HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const Faction& faction);
	Json toJson() const;
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void undesignate(Block& block);
	void remove(Block& block);
	void removeIfExists(Block& block);
	bool contains(const Block& block) const;
	const BlockFeatureType* at(const Block& block) const;
	bool empty() const;
	friend class HasConstructionDesignations;
};
// To be used by Area.
class HasConstructionDesignations final
{
	std::unordered_map<const Faction*, HasConstructionDesignationsForFaction> m_data;
public:
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(const Faction& faction);
	void removeFaction(const Faction& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const Faction& faction, Block& block, const BlockFeatureType* blockFeatureType, const MaterialType& materialType);
	void undesignate(const Faction& faction, Block& block);
	void remove(const Faction& faction, Block& block);
	void clearAll(Block& block);
	bool areThereAnyForFaction(const Faction& faction) const;
	bool contains(const Faction& faction, const Block& block) const;
	ConstructProject& getProject(const Faction& faction, Block& block);
	HasConstructionDesignationsForFaction& at(const Faction& faction) { return m_data.at(&faction); }
};
