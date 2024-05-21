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
	std::unordered_set<Actor*> m_actors;
	std::unordered_set<Actor*> m_onSurface;
public:
	LocationBuckets m_locationBuckets;
	VisionFacadeBuckets m_visionFacadeBuckets;
	OpacityFacade m_opacityFacade;
	AreaHasVisionCuboids m_visionCuboids;
	AreaHasActors(Area& a) : m_area(a), m_locationBuckets(a), m_visionFacadeBuckets(a), m_opacityFacade(a), m_visionCuboids() { }
	// Add to m_actors, m_locationBuckets, m_visionFacadeBuckets and possibly m_onSurface.
	// Run after initial location has been assigned.
	void add(Actor& actor);
	void remove(Actor& actor);
	// Update temperatures for all actors on surface.
	void onChangeAmbiantSurfaceTemperature();
	// Generate vision request queue and dispatch tasks.
	void processVisionReadStep();
	void processVisionWriteStep();
	void setUnderground(Actor& actor);
	void setNotUnderground(Actor& actor);
	[[nodiscard]] std::unordered_set<Actor*>& getAll() { return m_actors; }
	[[nodiscard]] const std::unordered_set<Actor*>& getAllConst() const { return m_actors; }
};
