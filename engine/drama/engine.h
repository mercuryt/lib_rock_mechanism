#pragma once
#include "../types.h"
#include "../config.h"
#include "../deserializationMemo.h"
#include "../index.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>
class Simulation;
class Area;
struct Shape;
class HasShape;
struct MoveType;
struct FluidType;
struct AnimalSpecies;
class DramaEngine;
enum class DramaArcType{AnimalsArrive, BanditsArrive};
class DramaArc
{
protected:
	DramaEngine& m_engine;
	// Optional
	Area* m_area = nullptr;
	DramaArc(DramaEngine& engine, DramaArcType type, Area* area = nullptr) : m_engine(engine), m_area(area), m_type(type) { }
	DramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	void actorsLeave(ActorIndices actors);
	[[nodiscard]] BlockIndex getEntranceToArea(const ShapeId& shape, const MoveTypeId& moveType) const;
	[[nodiscard]] BlockIndex findLocationOnEdgeForNear(const ShapeId& shape, const MoveTypeId& moveType, const BlockIndex& origin, const DistanceInBlocks& distance, const BlockIndices& exclude) const;
	[[nodiscard]] bool blockIsConnectedToAtLeast(const BlockIndex& block, const ShapeId& shape, const MoveTypeId& moveType, uint16_t count) const;
	[[nodiscard]] Facing4 getFacingAwayFromEdge(const BlockIndex& block) const;
	[[nodiscard]] std::vector<AnimalSpeciesId> getSentientSpecies() const;
	[[nodiscard]] virtual Json toJson() const;
	static std::unique_ptr<DramaArc> load(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	friend class DramaEngine;
public:
	DramaArcType m_type;
	std::string name() const { return typeToString(m_type); }
	//For debug.
	virtual void begin() = 0;
	static std::vector<DramaArcType> getTypes();
	static std::string typeToString(DramaArcType type);
	static DramaArcType stringToType(std::string string);
	virtual ~DramaArc() = default;
};
class DramaEngine final
{
public:
	DramaEngine(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation);
	DramaEngine(Simulation& simulation) : m_simulation(simulation) { }
	[[nodiscard]] Json toJson() const;
	void add(std::unique_ptr<DramaArc> event);
	void remove(DramaArc& event);
	void removeArcsForArea(Area& area);
	void createArcsForArea(Area& area);
	void createArcTypeForArea(DramaArcType type, Area& area);
	void removeArcTypeFromArea(DramaArcType type, Area& area);
	[[nodiscard]] std::vector<DramaArc*>& getArcsForArea(Area& area);
	[[nodiscard]] Simulation& getSimulation() { return m_simulation; }
	~DramaEngine();
private:
	Simulation& m_simulation;
	std::vector<std::unique_ptr<DramaArc>> m_arcs;
	AreaIdMap<std::vector<DramaArc*>> m_arcsByArea;
};
