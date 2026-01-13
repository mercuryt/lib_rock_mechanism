#pragma once

#include "../geometry/cuboid.h"
//#include "input.h"
#include "../reservable.h"
#include "../numericTypes/types.h"
#include "../eventSchedule.hpp"
#include "../project.h"
#include "../config/config.h"
#include "../projects/dig.h"

#include <vector>

struct Faction;
struct PointFeatureType;
class HasDigDesignationsForFaction;
struct DeserializationMemo;

class HasDigDesignationsForFaction final
{
	FactionId m_faction;
	SmallMapStable<Point3D, DigProject> m_data;
public:
	HasDigDesignationsForFaction(const FactionId& p) : m_faction(p) { }
	HasDigDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, const FactionId& faction, Area& area);
	void loadWorkers(const Json& data, DeserializationMemo& deserializationMemo);
	void designate(Area& area, const Point3D& point, const PointFeatureTypeId pointFeatureType);
	void undesignate(const Point3D& point);
	// To be called by undesignate as well as by DigProject::onCancel.
	void remove(Area& area, const Point3D& point);
	void removeIfExists(Area& area, const Point3D& point);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] PointFeatureTypeId getForPoint(const Point3D& point) const;
	[[nodiscard]] bool empty() const;
	[[nodiscard]] DigProject* getProjectWithCondition(const auto& shape, auto&& condition)
	{
		for(const auto& [point, project] : m_data)
			if(shape.contains(point) && condition(*project))
				return project.get();
		return nullptr;
	}
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
	// If pointFeatureType is null then dig out fully rather then digging out a feature.
	void designate(const FactionId& faction, const Point3D& point, const PointFeatureTypeId pointFeatureType);
	void undesignate(const FactionId& faction, const Point3D& point);
	void remove(const FactionId& faction, const Point3D& point);
	void clearAll(const Point3D& point);
	void clearReservations();
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] bool areThereAnyForFaction(const FactionId& faction) const;
	[[nodiscard]] bool contains(const FactionId& faction, const Point3D& point) const { return m_data[faction].m_data.contains(point); }
	[[nodiscard]] DigProject& getForFactionAndPoint(const FactionId& faction, const Point3D& point);
	[[nodiscard]] DigProject* getProjectWithCondition(const FactionId& faction, const auto& shape, auto&& condition) { return m_data[faction].getProjectWithCondition(shape, condition); }
};