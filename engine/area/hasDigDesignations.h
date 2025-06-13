#pragma once

#include "geometry/cuboid.h"
//#include "input.h"
#include "reservable.h"
#include "types.h"
#include "eventSchedule.hpp"
#include "project.h"
#include "config.h"
#include "../projects/dig.h"

#include <vector>

struct Faction;
struct BlockFeatureType;
class HasDigDesignationsForFaction;
struct DeserializationMemo;

class HasDigDesignationsForFaction final
{
	FactionId m_faction;
	SmallMapStable<BlockIndex, DigProject> m_data;
public:
	HasDigDesignationsForFaction(const FactionId& p) : m_faction(p) { }
	HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void designate(Area& area, const BlockIndex& block, const BlockFeatureTypeId blockFeatureType);
	void undesignate(const BlockIndex& block);
	// To be called by undesignate as well as by DigProject::onCancel.
	void remove(Area& area, const BlockIndex& block);
	void removeIfExists(Area& area, const BlockIndex& block);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] BlockFeatureTypeId getForBlock(const BlockIndex& block) const;
	[[nodiscard]] bool empty() const;
	friend class AreaHasDigDesignations;
};
// To be used by Area.
class AreaHasDigDesignations final
{
	Area& m_area;
	//TODO: Why must this be stable?
	SmallMapStable<FactionId, HasDigDesignationsForFaction> m_data;
public:
	AreaHasDigDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void addFaction(const FactionId& faction);
	void removeFaction(const FactionId& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureTypeId blockFeatureType);
	void undesignate(const FactionId& faction, const BlockIndex& block);
	void remove(const FactionId& faction, const BlockIndex& block);
	void clearAll(const BlockIndex& block);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool areThereAnyForFaction(const FactionId& faction) const;
	[[nodiscard]] bool contains(const FactionId& faction, const BlockIndex& block) const { return m_data[faction].m_data.contains(block); }
	[[nodiscard]] DigProject& getForFactionAndBlock(const FactionId& faction, const BlockIndex& block);
};