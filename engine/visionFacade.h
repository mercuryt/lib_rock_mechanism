#pragma once
#include "config.h"
#include "types.h"
#include "index.h"
#include <cassert>
#include <cstdint>
#include <vector>
class Area;
class Actor;
struct VisionCuboid;
// Holds Actor pointers along with their vision range and location for high speed iteration during vision read step.
// Also holds result.
class VisionFacade final
{
	Area* m_area;
	std::vector<ActorIndex> m_actors;
	std::vector<BlockIndex> m_locations;
	std::vector<DistanceInBlocks> m_ranges;
	std::vector<ActorIndices> m_results;
public:
	VisionFacade();
	// Used as part of initalizaton.
	void setArea(Area& area);
	void addActor(ActorIndex actor);
	void removeActor(ActorIndex actor);
	// TODO: What is this for? Used in clean-up? But why?
	void remove(VisionFacadeIndex index);
	void updateRange(VisionFacadeIndex index, DistanceInBlocks range);
	void updateLocation(VisionFacadeIndex index, BlockIndex& location);
	void readStepSegment(VisionFacadeIndex begin, VisionFacadeIndex end);
	void doStep();
	void readStep();
	void writeStep();
	void clear();
	void updateActorIndex(VisionFacadeIndex index, ActorIndex newIndex) { m_actors.at(index) = newIndex; }
	[[nodiscard]] ActorIndex getActor(VisionFacadeIndex index);
	[[nodiscard]] BlockIndex getLocation(VisionFacadeIndex index);
	[[nodiscard]] DistanceInBlocks getRange(VisionFacadeIndex index) const;
	[[nodiscard]] ActorIndices& getResults(VisionFacadeIndex index);
	[[nodiscard]] VisionFacadeIndex size() const { return m_actors.size(); }
	[[nodiscard]] static DistanceInBlocks taxiDistance(Point3D a, Point3D b);
};
// Divide actors into buckets by id.
// Read step is called for only one bucket per simulaiton step, cycling through them.
class VisionFacadeBuckets final
{
	Area& m_area;
	std::array<VisionFacade, Config::actorDoVisionInterval> m_data;
	VisionFacade& facadeForActor(ActorIndex actor);
public:
	VisionFacadeBuckets(Area& area);
	void add(ActorIndex actor);
	void remove(ActorIndex actor);
	void clear();
	[[nodiscard]] VisionFacade& getForStep(Step step);
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
	void create(Area& area, ActorIndex actor);
	// Call on sleep or blinding.
	void clear();
	// Call when vision range changes.
	void updateRange(DistanceInBlocks range);
	// Call when move.
	void updateLocation(BlockIndex& location);
	// Call when visionFacadeIndex changes.
	void updateFacadeIndex(VisionFacadeIndex index) { m_index = index; }
	void updateActorIndex(ActorIndex newIndex) { m_visionFacade->updateActorIndex(m_index, newIndex); }
	[[nodiscard]] bool empty() const { return m_index == VISION_FACADE_INDEX_MAX; }
	[[nodiscard]] VisionFacadeIndex getIndex() const { return m_index; }
	friend class VisionFacade;
	VisionFacade& getVisionFacade() const { return *m_visionFacade; }
};
