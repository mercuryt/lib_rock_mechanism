#pragma once

#include "geometry/cuboid.h"
#include "numericTypes/index.h"
#include "input.h"
#include "objective.h"
#include "project.h"
#include "path/pathRequest.h"
#include "path/terrainFacade.h"
#include "numericTypes/types.h"
#include "projects/woodcutting.h"
#include <memory>

struct WoodCuttingLocationDishonorCallback final : public DishonorCallback
{
	FactionId m_faction;
	Area& m_area;
	Point3D m_location;
	WoodCuttingLocationDishonorCallback(FactionId f, Area& a, const Point3D& l) : m_faction(f), m_area(a), m_location(l) { }
	WoodCuttingLocationDishonorCallback(const Json& data, DeserializationMemo& deserializationMemo);
	void execute(const Quantity& oldCount, const Quantity& newCount);
	[[nodiscard]] Json toJson() const;
};
// Part of HasWoodCuttingDesignations.
class HasWoodCuttingDesignationsForFaction final
{
	Area& m_area;
	FactionId m_faction;
	SmallMapStable<Point3D, WoodCuttingProject> m_data;
public:
	HasWoodCuttingDesignationsForFaction(FactionId p, Area& a) : m_area(a), m_faction(p) { }
	HasWoodCuttingDesignationsForFaction(const Json& data, DeserializationMemo& deserializationMemo, FactionId faction);
	Json toJson() const;
	void designate(const Point3D& point);
	void undesignate(const Point3D& point);
	// To be called by undesignate as well as by WoodCuttingProject::onCancel.
	void remove(const Point3D& point);
	void removeIfExists(const Point3D& point);
	bool empty() const;
	[[nodiscard]] WoodCuttingProject& getForPoint(const Point3D& point);
	[[nodiscard]] WoodCuttingProject* getProjectWithCondition(const auto& shape, auto&& condition)
	{
		for(const auto& [point, project] : m_data)
			if(shape.contains(point) && condition(*project))
				return project.get();
		return nullptr;
	}
	friend class AreaHasWoodCuttingDesignations;
};
// To be used by Area.
class AreaHasWoodCuttingDesignations final
{
	Area& m_area;
	SmallMapStable<FactionId, HasWoodCuttingDesignationsForFaction> m_data;
public:
	AreaHasWoodCuttingDesignations(Area& a) : m_area(a) { }
	void load(const Json& data, DeserializationMemo& deserializationMemo);
	Json toJson() const;
	void addFaction(FactionId faction);
	void removeFaction(FactionId faction);
	// TODO: designate and undesignate should probably take plants rather then space as arguments.
	void designate(FactionId faction, const Point3D& point);
	void undesignate(FactionId faction, const Point3D& point);
	void remove(FactionId faction, const Point3D& point);
	void clearAll(const Point3D& point);
	void clearReservations();
	bool areThereAnyForFaction(FactionId faction) const;
	bool contains(FactionId faction, const Point3D& point) const;
	HasWoodCuttingDesignationsForFaction& getForFaction(FactionId faction);
	const HasWoodCuttingDesignationsForFaction& getForFaction(FactionId faction) const;
};