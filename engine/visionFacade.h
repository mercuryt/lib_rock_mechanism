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
// Holds Actor pointers along with their vision range and location for high speed iteration during vision read step.
// Also holds result.
class VisionFacade final
{
	Area* m_area;
	std::vector<Actor*> m_actors;
	std::vector<Block*> m_locations;
	std::vector<DistanceInBlocks> m_ranges;
	std::vector<std::unordered_set<Actor*>> m_results;
public:
	VisionFacade();
	// Used as part of initalizaton.
	void setArea(Area& area);
	void add(Actor& actor);
	void remove(Actor& actor);
	void remove(size_t index);
	void updateRange(size_t index, DistanceInBlocks range);
	void updateLocation(size_t index, Block& location);
	void readStepSegment(size_t begin, size_t end);
	void readStep();
	void writeStep();
	void clear();
	[[nodiscard]] Actor& getActor(size_t index);
	[[nodiscard]] Block& getLocation(size_t index);
	[[nodiscard]] DistanceInBlocks getRange(size_t index) const;
	[[nodiscard]] std::unordered_set<Actor*>& getResults(size_t index);
	[[nodiscard]] size_t size() const { return m_actors.size(); }
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
	size_t m_index = SIZE_MAX;
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
	[[nodiscard]] bool empty() const { return m_index == SIZE_MAX; }
	friend class VisionFacade;
	VisionFacade& getVisionFacade() const { return *m_visionFacade; }
};
