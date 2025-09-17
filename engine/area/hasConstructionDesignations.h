#pragma once
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "../projects/construct.h"

class DeserializationMemo;

class HasConstructionDesignationsForFaction final
{
	FactionId m_faction;
	//TODO: More then one construct project targeting a given point should be able to exist simultaniously.
	SmallMapStable<Point3D, ConstructProject> m_data;
public:
	HasConstructionDesignationsForFaction(const FactionId& p) : m_faction(p) { }
	HasConstructionDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	// If pointFeatureType is null then construct a wall rather then a feature.
	void designate(Area& area, const Point3D& point, const PointFeatureTypeId pointFeatureType, const MaterialTypeId& materialType);
	void undesignate(const Point3D& point);
	void remove(Area& area, const Point3D& point);
	void removeIfExists(Area& area, const Point3D& point);
	[[nodiscard]] bool contains(const Point3D& point) const;
	[[nodiscard]] PointFeatureTypeId getForPoint(const Point3D& point) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] ConstructProject* getProjectWithCondition(const auto& shape, auto&& condition)
	{
		for(const auto& [point, project] : m_data)
			if(shape.contains(point) && condition(*project))
				return project.get();
		return nullptr;
	}
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
	[[nodiscard]] Json toJson() const;
	void addFaction(const FactionId& faction);
	void removeFaction(const FactionId& faction);
	// If pointFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const FactionId& faction, const Point3D& point, const PointFeatureTypeId pointFeatureType, const MaterialTypeId& materialType);
	void undesignate(const FactionId& faction, const Point3D& point);
	void remove(const FactionId& faction, const Point3D& point);
	void clearAll(const Point3D& point);
	void clearReservations();
	[[nodiscard]] bool areThereAnyForFaction(const FactionId& faction) const;
	[[nodiscard]] bool contains(const FactionId& faction, const Point3D& point) const;
	[[nodiscard]] ConstructProject& getProject(const FactionId& faction, const Point3D& point);
	[[nodiscard]] ConstructProject* getProjectWithCondition(const FactionId& faction, const auto& shape, auto&& condition) { return m_data[faction].getProjectWithCondition(shape, condition); }
	[[nodiscard]] HasConstructionDesignationsForFaction& getForFaction(const FactionId& faction) { return m_data[faction]; }
};