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
	InstallItemProject(Area& area, const ItemReference& i, const BlockIndex& l, const Facing& facing, const FactionId& faction);
	void onComplete();
	std::vector<std::pair<ItemQuery, Quantity>> getConsumed() const { return {}; }
	std::vector<std::pair<ItemQuery, Quantity>> getUnconsumed() const { return {{ItemQuery::create(m_item), Quantity::create(1)}}; }
	std::vector<std::pair<ActorQuery, Quantity>> getActors() const { return {}; }
	std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>> getByproducts() const { return {}; }
	Step getDuration() const { return Config::installItemDuration; }
	bool canReset() const { return false; }
	void onDelay() { cancel(); }
	void offDelay() { assert(false); }
};
class HasInstallItemDesignationsForFaction final
{
	std::unordered_map<BlockIndex, InstallItemProject, BlockIndex::Hash> m_designations;
	FactionId m_faction;
public:
	HasInstallItemDesignationsForFaction(const FactionId& faction) : m_faction(faction) { }
	void add(Area& area, const BlockIndex& block, const ItemIndex& item, const Facing& facing, const FactionId& faction);
	void remove(Area& area, const ItemIndex& item);
	bool empty() const { return m_designations.empty(); }
	bool contains(const BlockIndex& block) const { return m_designations.contains(block); }
	InstallItemProject& getForBlock(const BlockIndex& block) { return m_designations.at(block); }
	friend class AreaHasInstallItemDesignations;
};
class AreaHasInstallItemDesignations final
{
	std::unordered_map<FactionId, HasInstallItemDesignationsForFaction, FactionId::Hash> m_data;
public:
	void registerFaction(const FactionId& faction) { m_data.emplace(faction, faction); }
	void unregisterFaction(const FactionId& faction) { m_data.erase(faction); }
	void clearReservations();
	HasInstallItemDesignationsForFaction& getForFaction(const FactionId& faction) { assert(m_data.contains(faction)); return m_data.at(faction);}
};
