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
	Item& m_item;
	Facing m_facing;
public:
	// Max one worker.
	InstallItemProject(Item& i, Block& l, Facing facing, const Faction& faction) : Project(&faction, l, 1), m_item(i), m_facing(facing) { }
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
	InstallItemThreadedTask(InstallItemObjective& iio);
	// Search for nearby item that needs installing.
	void readStep();
	void writeStep();
	void clearReferences();
};
class InstallItemObjective final : public Objective
{
	HasThreadedTask<InstallItemThreadedTask> m_threadedTask;
	InstallItemProject* m_project;
public:
	InstallItemObjective(Actor& a);
	InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(); 
	void cancel() { m_threadedTask.maybeCancel(); m_project->removeWorker(m_actor); }
	void delay() { }
	void reset();
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::InstallItem; }
	std::string name() const { return "install item"; }
	friend class InstallItemThreadedTask;
};
class InstallItemObjectiveType final : public ObjectiveType
{
public:
	bool canBeAssigned(Actor& actor) const;
	std::unique_ptr<Objective> makeFor(Actor& actor) const;
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::InstallItem; }
};
class HasInstallItemDesignationsForFaction final
{
	std::unordered_map<Block*, InstallItemProject> m_designations;
	const Faction& m_faction;
public:
	HasInstallItemDesignationsForFaction(const Faction& faction) : m_faction(faction) { }
	void add(Block& block, Item& item, Facing facing, const Faction& faction);
	void remove(Item& item);
	bool empty() const { return m_designations.empty(); }
	bool contains(const Block& block) const { return m_designations.contains(&const_cast<Block&>(block)); }
	InstallItemProject& at(Block& block) { return m_designations.at(&block); }
	friend class AreaHasInstallItemDesignations;
};
class AreaHasInstallItemDesignations final
{
	std::unordered_map<const Faction*, HasInstallItemDesignationsForFaction> m_data;
public:
	void registerFaction(const Faction& faction) { m_data.try_emplace(&faction, faction); }
	void unregisterFaction(const Faction& faction) { m_data.erase(&faction); }
	void clearReservations();
	HasInstallItemDesignationsForFaction& at(const Faction& faction) { assert(m_data.contains(&faction)); return m_data.at(&faction);}
};
