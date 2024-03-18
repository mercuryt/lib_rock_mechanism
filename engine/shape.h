/*
 * A 3d shape defined by offsets and volume.
 */

#pragma once

#include "config.h"
#include "deserializationMemo.h"
#include "types.h"

#include <cassert>
#include <list> 
#include <vector> 
#include <array> 
#include <string>
#include <algorithm>
#include <set>

struct Shape
{
	const std::string name;
	std::vector<std::array<int32_t, 4>> positions;
	const uint32_t displayScale;
	const bool isMultiTile;
	std::array<std::vector<std::array<int32_t, 4>>,4> occupiedOffsetsCache;
	std::array<std::vector<std::array<int32_t, 3>>,4> adjacentOffsetsCache;
	[[nodiscard]] std::vector<std::array<int32_t, 4>> positionsWithFacing(Facing facing) const { return occupiedOffsetsCache.at(facing); }
	[[nodiscard]] std::vector<std::array<int32_t, 3>> adjacentPositionsWithFacing(Facing facing) const { return adjacentOffsetsCache.at(facing); }
	Shape(const std::string n, std::vector<std::array<int32_t, 4>> p, uint32_t ds);
	// For custom shapes.
	Shape(const Json& data, DeserializationMemo& deserializationMemo);
	[[nodiscard]] Json toJson() const;
	[[nodiscard]] std::vector<std::array<int32_t, 4>> makeOccupiedPositionsWithFacing(Facing facing) const;
	[[nodiscard]] std::vector<std::array<int32_t, 3>> makeAdjacentPositionsWithFacing(Facing facing) const;
	[[nodiscard]] std::vector<std::array<int32_t, 3>> positionOffsets(std::array<int32_t, 4> position) const;
	[[nodiscard]] std::vector<Block*> getBlocksOccupiedAt(const Block& location, Facing facing) const;
	[[nodiscard]] std::vector<std::pair<Block*, Volume>> getBlocksOccupiedAtWithVolumes(const Block& location, Facing facing) const;
	// Infastructure.
	bool operator==(const Shape& x) const { return &x == this; }
	inline static std::vector<Shape> data;
	static const Shape& byName(const std::string& name)
	{
		auto found = std::ranges::find(data, name, &Shape::name);
		assert(found != data.end());
		return *found;
	}
	static bool hasShape(const std::string& name)
	{
		auto found = std::ranges::find(data, name, &Shape::name);
		return found != data.end();
	}
};
inline void to_json(Json& data, const Shape* const& shape){ data = shape->name; }
inline void to_json(Json& data, const Shape& shape){ data = shape.name; }
inline void from_json(const Json& data, const Shape*& shape){ shape = &Shape::byName(data.get<std::string>()); }
// Store custom shapes.
class SimulationHasShapes final 
{
	std::unordered_map<std::string, const Shape> m_shapes;
	std::unordered_map<const Shape*, std::string> m_namesByShape;
public:
	SimulationHasShapes(const Json& data, DeserializationMemo& deserializationMemo);
	SimulationHasShapes() = default;
	Json toJson() const;
	void loadFromName(std::string name);
	// creates a copy, adds a position to it and returns it.
	const Shape& mutateAdd(const Shape& shape, std::array<int32_t, 4> position);
	const Shape& byName(std::string name) const;
	const std::string getName(const Shape& shape) const;
	const std::string makeName(std::vector<std::array<int32_t, 4>> positions) const;
	bool contains(std::string name) const { return m_shapes.contains(name); }
};
