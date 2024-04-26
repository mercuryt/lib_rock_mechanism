#pragma once
#include "../types.h"
#include "../config.h"
#include "../visionRequest.h"
#include "../locationBuckets.h"
#include "../buckets.h"

#include <unordered_set>

class Area;
class Actor;
class AreaHasActors
{
	Area& m_area;
	std::unordered_set<Actor*> m_actors;
	std::unordered_set<Actor*> m_onSurface;
	std::vector<VisionRequest> m_visionRequestQueue;
public:
	LocationBuckets m_locationBuckets;
	Buckets<Actor, Config::actorDoVisionInterval> m_visionBuckets;
	AreaHasActors(Area& a) : m_area(a), m_locationBuckets(a) { }
	// Add to m_actors, m_locationBuckets, m_visionBuckets and possibly m_onSurface.
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
	~AreaHasActors() { m_visionRequestQueue.clear(); }
	[[nodiscard]] std::unordered_set<Actor*>& getAll() { return m_actors; }
	[[nodiscard]] const std::unordered_set<Actor*>& getAllConst() const { return m_actors; }
};
