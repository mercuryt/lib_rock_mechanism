#pragma once
#include "project.h"
#include "objective.h"
#include "reference.h"
#include "path/pathRequest.h"
#include "projects/installItem.h"
#include "numericTypes/types.h"
#include "numericTypes/index.h"

#include <cstdio>
class HasInstallItemDesignationsForFaction final
{
	SmallMapStable<BlockIndex, InstallItemProject> m_designations;
	FactionId m_faction;
public:
	HasInstallItemDesignationsForFaction(const FactionId& faction) : m_faction(faction) { }
	void add(Area& area, const BlockIndex& block, const ItemIndex& item, const Facing4& facing, const FactionId& faction);
	void remove(Area& area, const ItemIndex& item);
	bool empty() const { return m_designations.empty(); }
	bool contains(const BlockIndex& block) const { return m_designations.contains(block); }
	InstallItemProject& getForBlock(const BlockIndex& block) { return m_designations[block]; }
	friend class AreaHasInstallItemDesignations;
};
class AreaHasInstallItemDesignations final
{
	// TODO: change to small map.
	std::unordered_map<FactionId, HasInstallItemDesignationsForFaction, FactionId::Hash> m_data;
public:
	void registerFaction(const FactionId& faction) { m_data.emplace(faction, faction); }
	void unregisterFaction(const FactionId& faction) { m_data.erase(faction); }
	void clearReservations();
	HasInstallItemDesignationsForFaction& getForFaction(const FactionId& faction) { assert(m_data.contains(faction)); return m_data.at(faction);}
};
