#pragma once
#include "config.h"
#include "types.h"
#include "index.h"
#include "dataVector.h"
#include <cassert>
#include <cstdint>
#include <vector>
class Area;
class Actor;
struct VisionCuboid;
// Holds Actor pointers along with their vision range and location for high speed iteration during vision read step.
class VisionFacade final
{
	DataVector<ActorIndex, VisionFacadeIndex> m_actors;
	DataVector<BlockIndex, VisionFacadeIndex> m_locations;
	DataVector<VisionCuboidId, VisionFacadeIndex> m_cuboids;
	DataVector<DistanceInBlocks, VisionFacadeIndex> m_ranges;
	DataVector<ActorIndices, VisionFacadeIndex> m_results;
	Area* m_area = nullptr;
public:
	VisionFacade();
	// Used as part of initalizaton.
	void setArea(Area& area);
	void addActor(const ActorIndex& actor);
	void removeActor(const ActorIndex& actor);
	// TODO: What is this for? Used in clean-up? But why?
	void remove(const VisionFacadeIndex& index);
	void updateRange(const VisionFacadeIndex& index, const DistanceInBlocks& range);
	void updateLocation(const VisionFacadeIndex& index, const BlockIndex& location);
	void updateVisionCuboid(const VisionFacadeIndex& index, const VisionCuboidId& cuboid);
	void readStepSegment(const VisionFacadeIndex& begin, const VisionFacadeIndex& end);
	void doStep();
	void readStep();
	void writeStep();
	void clear();
	// To be called when actor index changes due to deletion or sorting.
	void updateActorIndex(const VisionFacadeIndex& index, const ActorIndex& newIndex) { m_actors[index] = newIndex; }
	// These accessors get data from the internal DataVectors and assert they are still matching the cannonical data in Actors and AreaHasVisionCuboids.
	[[nodiscard]] ActorIndex getActor(const VisionFacadeIndex& index) const;
	[[nodiscard]] BlockIndex getLocation(const VisionFacadeIndex& index) const;
	[[nodiscard]] DistanceInBlocks getRange(const VisionFacadeIndex& index) const;
	[[nodiscard]] VisionCuboidId getCuboid( const VisionFacadeIndex& index) const;
	[[nodiscard]] ActorIndices& getResults(const VisionFacadeIndex& index) const;
	[[nodiscard]] size_t size() const { return m_actors.size(); }
	[[nodiscard]] static DistanceInBlocks taxiDistance(Point3D a, Point3D b);
};
// Divide actors into buckets by id.
// Step is called for only one bucket per simulaiton step, cycling through them.
class VisionFacadeBuckets final
{
	Area& m_area;
	std::array<VisionFacade, Config::actorDoVisionInterval> m_data;
public:
	VisionFacadeBuckets(Area& area);
	void add(const ActorIndex& actor);
	void remove(const ActorIndex& actor);
	void clear();
	void doStep(const Step& step) { getForStep(step).doStep(); }
	// When a block's vision cuboid changes we must update the stored vision cuboid in the vision facade for all actors located on that block.
	void updateCuboid(const ActorIndex& actor, const VisionCuboidId& visionCuboidId);
	[[nodiscard]] VisionFacade& getForStep(Step step);
	[[nodiscard]] VisionFacade& getForActor(const ActorIndex& actor);
};
// RAII handle for vision facade data.
// Stores m_index privately with no accessor, can only be updated by it's friend class VisionFacade.
// Used to create, update and destroy facade data.
class HasVisionFacade final
{
	VisionFacade* m_visionFacade = nullptr;
	VisionFacadeIndex m_index;
public:
	// To be called when leaving an area. Also calls clear if !empty().
	void clearVisionFacade();
	// record bucket.
	void initalize(Area& area, const ActorIndex& actor);
	// Call on actor create, arrive in area, wakeup, get out of vechicle, etc. if can currently see
	void create(Area& area, const ActorIndex& actor);
	// Call on sleep, enter area, or blinding.
	void clear();
	// Call when vision range changes.
	void updateRange(const DistanceInBlocks& range);
	// Call when move.
	void updateLocation(const BlockIndex& location);
	// Call when actor index changes.
	void updateActorIndex(const ActorIndex& newIndex) { m_visionFacade->updateActorIndex(m_index, newIndex); }
	[[nodiscard]] bool empty() const { return m_index.empty(); }
	[[nodiscard]] VisionFacadeIndex getIndex() const { return m_index; }
	friend class VisionFacade;
	// For testing.
	[[nodiscard]] VisionFacade& getVisionFacade() const { return *m_visionFacade; }
};
