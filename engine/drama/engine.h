#pragma once
#include "../types.h"
#include "../config.h"
#include "../deserializationMemo.h"
#include "../blockIndices.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
class Simulation;
class Area;
class Actor;
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
	Area* m_area;
	DramaArc(DramaEngine& engine, DramaArcType type, Area* area = nullptr) : m_engine(engine), m_area(area), m_type(type) { }
	DramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	void actorsLeave(ActorIndices actors);
	[[nodiscard]] BlockIndex getEntranceToArea(const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] BlockIndex findLocationOnEdgeForNear(const Shape& shape, const MoveType& moveType, BlockIndex origin, DistanceInBlocks distance, BlockIndices& exclude) const;
	[[nodiscard]] bool blockIsConnectedToAtLeast(BlockIndex block, const Shape& shape, const MoveType& moveType, uint16_t count) const;
	[[nodiscard]] Facing getFacingAwayFromEdge(BlockIndex block) const;
	[[nodiscard]] std::vector<const AnimalSpecies*> getSentientSpecies() const;
	[[nodiscard]] virtual Json toJson() const;
	static std::unique_ptr<DramaArc> load(const Json& data, DeserializationMemo& deserializationMemo,DramaEngine& dramaEngine);
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
	[[nodiscard]] std::unordered_set<DramaArc*>& getArcsForArea(Area& area);
	[[nodiscard]] Simulation& getSimulation() { return m_simulation; }
	~DramaEngine();
private:
	Simulation& m_simulation;
	std::vector<std::unique_ptr<DramaArc>> m_arcs;
	std::unordered_map<Area*, std::unordered_set<DramaArc*>> m_arcsByArea;
};
