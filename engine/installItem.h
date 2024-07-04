#pragma once
#include "pathRequest.h"
#include "project.h"
#include "objective.h"

#include <cstdio>
struct DeserializationMemo;
struct FindPathResult;
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
// Search for nearby item that needs installing.
class InstallItemPathRequest final : public PathRequest
{
	InstallItemObjective& m_installItemObjective;
public:
	InstallItemPathRequest(Area& area, InstallItemObjective& iio);
	void callback(Area& area, FindPathResult& result);
};
class InstallItemObjective final : public Objective
{
	InstallItemProject* m_project;
public:
	InstallItemObjective(ActorIndex a);
	InstallItemObjective(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void execute(Area& area); 
	void cancel(Area& area);
	void delay(Area&) { }
	void reset(Area& area);
	ObjectiveTypeId getObjectiveTypeId() const { return ObjectiveTypeId::InstallItem; }
	std::string name() const { return "install item"; }
	friend class InstallItemPathRequest;
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
	void add(Area& area, BlockIndex block, ItemIndex item, Facing facing, Faction& faction);
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
