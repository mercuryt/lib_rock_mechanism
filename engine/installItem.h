#pragma once
#include "index.h"
#include "project.h"
#include "objective.h"
#include "pathRequest.h"
#include "reference.h"
#include "types.h"

#include <cstdio>
struct DeserializationMemo;
struct FindPathResult;
class InstallItemObjective;
class InstallItemProject final : public Project
{
	const ItemReference m_item;
	Facing m_facing;
public:
	// Max one worker.
	InstallItemProject(Area& area, ItemReference i, BlockIndex l, Facing facing, FactionId faction);
	void onComplete();
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const { ItemQuery query(m_item); return {{m_item,Quantity::create(1)}}; }
	std::vector<std::pair<ActorQuery, Quantity>> getActors() const { return {}; }
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const { return {}; }
	Step getDuration() const { return Config::installItemDuration; }
	bool canReset() const { return false; }
	void onDelay() { cancel(); }
	void offDelay() { assert(false); }
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
	BlockIndexMap<InstallItemProject> m_designations;
	FactionId m_faction;
public:
	HasInstallItemDesignationsForFaction(FactionId faction) : m_faction(faction) { }
	void add(Area& area, BlockIndex block, ItemIndex item, Facing facing, FactionId faction);
	void remove(Area& area, ItemIndex item);
	bool empty() const { return m_designations.empty(); }
	bool contains(const BlockIndex block) const { return m_designations.contains(block); }
	InstallItemProject& at(BlockIndex block) { return m_designations.at(block); }
	friend class AreaHasInstallItemDesignations;
};
class AreaHasInstallItemDesignations final
{
	FactionIdMap<HasInstallItemDesignationsForFaction> m_data;
public:
	void registerFaction(FactionId faction) { m_data.try_emplace(faction, faction); }
	void unregisterFaction(FactionId faction) { m_data.erase(faction); }
	void clearReservations();
	HasInstallItemDesignationsForFaction& at(FactionId faction) { assert(m_data.contains(faction)); return m_data.at(faction);}
};
