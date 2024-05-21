#pragma once
#include "config.h"
#include "types.h"
#include <TGUI/Event.hpp>
#include <cassert>
#include <cstdint>
#include <vector>
#include <unordered_set>
class Area;
class Block;
class Actor;
struct VisionCuboid;
// Holds Actor pointers along with their vision range and location for high speed iteration during vision read step.
// Also holds result.
class VisionFacade final
{
	Area* m_area;
	std::vector<Actor*> m_actors;
	std::vector<BlockIndex> m_locations;
	std::vector<DistanceInBlocks> m_ranges;
	std::vector<std::vector<Actor*>> m_results;
public:
	VisionFacade();
	// Used as part of initalizaton.
	void setArea(Area& area);
	void add(Actor& actor);
	void remove(Actor& actor);
	void remove(VisionFacadeIndex index);
	void updateRange(VisionFacadeIndex index, DistanceInBlocks range);
	void updateLocation(VisionFacadeIndex index, Block& location);
	void readStepSegment(VisionFacadeIndex begin, VisionFacadeIndex end);
	void readStep();
	void writeStep();
	void clear();
	[[nodiscard]] Actor& getActor(VisionFacadeIndex index);
	[[nodiscard]] BlockIndex getLocation(VisionFacadeIndex index);
	[[nodiscard]] DistanceInBlocks getRange(VisionFacadeIndex index) const;
	[[nodiscard]] std::vector<Actor*>& getResults(VisionFacadeIndex index);
	[[nodiscard]] VisionFacadeIndex size() const { return m_actors.size(); }
	[[nodiscard]] static DistanceInBlocks taxiDistance(Point3D a, Point3D b);
};
// Divide actors into buckets by id.
// Read step is called for only one bucket per simulaiton step, cycling through them.
class VisionFacadeBuckets final
{
	std::array<VisionFacade, Config::actorDoVisionInterval> m_data;
	VisionFacade& facadeForActor(Actor& actor);
public:
	VisionFacadeBuckets(Area& area);
	void add(Actor& actor);
	void remove(Actor& actor);
	void clear();
	VisionFacade& getForStep(Step step);
};
// RAII handle for vision facade data.
// Stores m_index privately with no accessor, can only be updated by it's friend class VisionFacade.
// Used to create, update and destroy facade data.
class HasVisionFacade final
{
	VisionFacade* m_visionFacade = nullptr;
	VisionFacadeIndex m_index = VISION_FACADE_INDEX_MAX;
public:
	// To be called when leaving an area. Also calls clear if !empty().
	void clearVisionFacade();
	// Create facade data, to be used when actor enters area, wakes up, or temporary blindness ends.
	void create(Actor& actor);
	// Call on sleep or blinding.
	void clear();
	// Call when vision range changes.
	void updateRange(DistanceInBlocks range);
	// Call when move.
	void updateLocation(Block& location);
	[[nodiscard]] bool empty() const { return m_index == VISION_FACADE_INDEX_MAX; }
	friend class VisionFacade;
	VisionFacade& getVisionFacade() const { return *m_visionFacade; }
};
