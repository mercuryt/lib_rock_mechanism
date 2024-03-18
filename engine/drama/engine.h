#pragma once
#include "../types.h"
#include "../config.h"
#include "../deserializationMemo.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
class Area;
class Block;
class Actor;
struct Shape;
class HasShape;
struct MoveType;
struct FluidType;
class DramaEngine;
enum class DramaArcType{AnimalsArrive};
class DramaArc
{
protected:
	DramaEngine& m_engine;
	// Optional
	Area* m_area;
	DramaArc(DramaEngine& engine, DramaArcType type, Area* area = nullptr) : m_engine(engine), m_area(area), m_type(type) { }
	DramaArc(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] virtual Json toJson() const;
	void actorsLeave(std::vector<Actor*> actors);
	[[nodiscard]] Block* getEntranceToArea(Area& area, const Shape& shape, const MoveType& moveType) const;
	[[nodiscard]] Block* findLocationOnEdgeForNear(const Shape& shape, const MoveType& moveType, Block& origin, DistanceInBlocks distance, std::unordered_set<Block*> exclude) const;
	[[nodiscard]] bool blockIsConnectedToAtLeast(const Block& block, const Shape& shape, const MoveType& moveType, uint16_t count) const;
	[[nodiscard]] Facing getFacingAwayFromEdge(const Block& block) const;
	static std::unique_ptr<DramaArc> load(const Json& data, DeserializationMemo& deserializationMemo);
	friend class DramaEngine;
public:
	DramaArcType m_type;
	std::string name() const { return typeToString(m_type); }
	//For debug.
	virtual void begin() = 0;
	static std::vector<DramaArcType> getTypes();
	static std::string typeToString(DramaArcType type);
	static DramaArcType stringToType(std::string string);
};
class DramaEngine final
{
public:
	DramaEngine(const Json& data, DeserializationMemo& deserializationMemo);
	DramaEngine() = default;
	[[nodiscard]] Json toJson() const;
	void add(std::unique_ptr<DramaArc> event);
	void remove(DramaArc& event);
	void removeArcsForArea(Area& area);
	void createArcsForArea(Area& area);
	void createArcTypeForArea(DramaArcType type, Area& area);
	[[nodiscard]] std::unordered_set<DramaArc*>& getArcsForArea(Area& area);
private:
	std::vector<std::unique_ptr<DramaArc>> m_arcs;
	std::unordered_map<Area*, std::unordered_set<DramaArc*>> m_arcsByArea;
};
