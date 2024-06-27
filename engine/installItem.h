#pragma once
#include "findsPath.h"
#include "project.h"
#include "threadedTask.h"
#include "objective.h"

#include <cstdio>
struct DeserializationMemo;
class InstallItemObjective;
class InstallItemProject final : public Project
{
	ItemIndex m_item;
	Facing m_facing;
public:
	// Max one worker.
	InstallItemProject(Area& area, ItemIndex i, BlockIndex l, Facing facing, Faction& faction);
	void onComplete();
	std::vector<std::pair<ItemQuery, uint32_t>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, uint32_t>> getUnconsumed() const { return {{m_item,1}}; }
	std::vector<std::pair<ActorQuery, uint32_t>> getActors() const { return {}; }
	std::vector<std::tuple<const ItemType*, const MaterialType*, uint32_t>> getByproducts() const { return {}; }
	Step getDuration() const { return Config::installItemDuration; }
	bool canReset() const { return false; }
	void onDelay() { cancel(); }
	void offDelay() { assert(false); }
};
class InstallItemThreadedTask final : public ThreadedTask
{
	InstallItemObjective& m_installItemObjective;
	FindsPath m_findsPath;
public:
	InstallItemThreadedTask(Area& area, InstallItemObjective& iio);
	// Search for nearby item that needs installing.
	void readStep(Simulation& simulation, Area* area);
	void writeStep(Simulation& simulation, Area* area);
	void clearReferences(Simulation& simulation, Area* area);
};
class InstallItemObjective final : public Objective
{
	HasThreadedTask<InstallItemThreadedTask> m_threadedTask;
	InstallItemProject* m_project;
public:
	InstallItemObjective(Area& area, ActorIndex a);
	InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area); 
	void cancel(Area& area);
	void delay(Area&) { }
	void reset(Area& area);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::InstallItem; }
	std::string name() const { return "install item"; }
	friend class InstallItemThreadedTask;
};
class InstallItemObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Area& area, ActorIndex actor) const;
	std::unique_ptr<Objective> makeFor(Area& area, ActorIndex actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::InstallItem; }
};
class HasInstallItemDesignationsForFaction final
{
	std::unordered_map<BlockIndex, InstallItemProject> m_designations;
	Faction& m_faction;
public:
	HasInstallItemDesignationsForFaction(Faction& faction) : m_faction(faction) { }
	void add(BlockIndex block, ItemIndex item, Facing facing, Faction& faction);
	void remove(Area& area, ItemIndex item);
	bool empty() const { return m_designations.empty(); }
	bool contains(const BlockIndex block) const { return m_designations.contains(block); }
	InstallItemProject& at(BlockIndex block) { return m_designations.at(block); }
	friend class AreaHasInstallItemDesignations;
};
class AreaHasInstallItemDesignations final
{
	std::unordered_map<Faction*, HasInstallItemDesignationsForFaction> m_data;
public:
	void registerFaction(Faction& faction) { m_data.try_emplace(&faction, faction); }
	void unregisterFaction(Faction& faction) { m_data.erase(&faction); }
	void clearReservations();
	HasInstallItemDesignationsForFaction& at(Faction& faction) { assert(m_data.contains(&faction)); return m_data.at(&faction);}
};
