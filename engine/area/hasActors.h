#pragma once
#include "../types.h"
#include "../config.h"
#include "../locationBuckets.h"
#include "../visionFacade.h"
#include "../opacityFacade.h"
#include "../visionCuboid.h"

#include <unordered_set>
#include <vector>

class Area;
class Actor;
class AreaHasActors
{
	Area& m_area;
	std::unordered_set<ActorIndex> m_actors;
	std::unordered_set<ActorIndex> m_onSurface;
public:
	AreaHasActors(Area& a) : m_area(a) { }
	// Add to m_actors, m_locationBuckets, m_visionFacadeBuckets and possibly m_onSurface.
	// Run after initial location has been assigned.
	void add(ActorIndex actor);
	void remove(ActorIndex actor);
	// Update temperatures for all actors on surface.
	void onChangeAmbiantSurfaceTemperature();
	void setUnderground(ActorIndex actor);
	void setNotUnderground(ActorIndex actor);
	[[nodiscard]] std::unordered_set<ActorIndex>& getAll() { return m_actors; }
	[[nodiscard]] const std::unordered_set<ActorIndex>& getAllConst() const { return m_actors; }
};
