#pragma once
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "../projects/construct.h"

class DeserializationMemo;

class HasConstructionDesignationsForFaction final
{
	FactionId m_faction;
	//TODO: More then one construct project targeting a given block should be able to exist simultaniously.
	SmallMapStable<BlockIndex, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(const FactionId& p) : m_faction(p) { }
	HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	// If blockFeatureType is null then construct a wall rather then a feature.
	void designate(Area& area, const BlockIndex& block, const BlockFeatureTypeId blockFeatureType, const MaterialTypeId& materialType);
	void undesignate(const BlockIndex& block);
	void remove(Area& area, const BlockIndex& block);
	void removeIfExists(Area& area, const BlockIndex& block);
	bool contains(const BlockIndex& block) const;
	BlockFeatureTypeId getForBlock(const BlockIndex& block) const;
	bool empty() const;
	friend class AreaHasConstructionDesignations;
};
// To be used by Area.
class AreaHasConstructionDesignations final
{
	Area& m_area;
	//TODO: Why does this have to be stable?
	SmallMapStable<FactionId, HasConstructionDesignationsForFaction> m_data;
public:
	AreaHasConstructionDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(const FactionId& faction);
	void removeFaction(const FactionId& faction);
	// If blockFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const FactionId& faction, const BlockIndex& block, const BlockFeatureTypeId blockFeatureType, const MaterialTypeId& materialType);
	void undesignate(const FactionId& faction, const BlockIndex& block);
	void remove(const FactionId& faction, const BlockIndex& block);
	void clearAll(const BlockIndex& block);
	void clearReservations();
	bool areThereAnyForFaction(const FactionId& faction) const;
	bool contains(const FactionId& faction, const BlockIndex& block) const;
	ConstructProject& getProject(const FactionId& faction, const BlockIndex& block);
	HasConstructionDesignationsForFaction& getForFaction(const FactionId& faction) { return m_data[faction]; }
};