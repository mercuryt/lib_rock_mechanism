#pragma once
#include "../numericTypes/types.h"
#include "../numericTypes/index.h"
#include "../config/config.h"
#include "../deserializationMemo.h"
#include "../dataStructures/smallMap.h"
#include "../geometry/point3D.h"
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
enum class DramaArcType {
	AnimalsArrive,
	BanditsArrive,
	MistakeAtWork,
	SuccessAtWork,
	//AccidentalHomicide,
	//AccidentalInjury,
	InspirationalSpeach
};
class DramaArc
{
protected:
	DramaEngine& m_engine;
	// Optional
	Area* m_area = nullptr;
	DramaArc(DramaEngine& engine, DramaArcType type, Area* area = nullptr) : m_engine(engine), m_area(area), m_type(type) { }
	DramaArc(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	void actorsLeave(SmallSet<ActorIndex> actors);
	[[nodiscard]] Point3D getEntranceToArea(const ShapeId& shape, const MoveTypeId& moveType) const;
	[[nodiscard]] Point3D findLocationOnEdgeForNear(const ShapeId& shape, const MoveTypeId& moveType, const Point3D& origin, const Distance& distance, const SmallSet<Point3D>& exclude) const;
	[[nodiscard]] bool pointIsConnectedToAtLeast(const Point3D& point, const ShapeId& shape, const MoveTypeId& moveType, int16_t count) const;
	[[nodiscard]] Facing4 getFacingAwayFromEdge(const Point3D& point) const;
	[[nodiscard]] std::vector<AnimalSpeciesId> getSentientSpecies() const;
	[[nodiscard]] virtual Json toJson() const;
	static std::unique_ptr<DramaArc> load(const Json& data, DeserializationMemo& deserializationMemo, DramaEngine& dramaEngine);
	friend class DramaEngine;
public:
	DramaArcType m_type;
	[[nodiscard]] std::string name() const { return typeToString(m_type); }
	[[nodiscard]] Area* getArea() { return m_area; }
	[[nodiscard]] static std::vector<DramaArcType> getTypes();
	[[nodiscard]] static std::string typeToString(DramaArcType type);
	[[nodiscard]] static DramaArcType stringToType(std::string string);
	// This method doesn't have to be defined but if it isn't then it should never be called.
	virtual void actorGoesOffScript(Area&, const ActorIndex&) { std::unreachable(); }
	virtual ~DramaArc() = default;
};
class DramaEngine final
{
public:
	DramaEngine(const Json& data, DeserializationMemo& deserializationMemo, Simulation& simulation);
	DramaEngine(Simulation& simulation) : m_simulation(simulation) { }
	[[nodiscard]] Json toJson() const;
	void add(std::unique_ptr<DramaArc>&& event);
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
	SmallMap<AreaId, std::vector<DramaArc*>> m_arcsByArea;
};
