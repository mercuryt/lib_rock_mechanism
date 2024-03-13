#pragma once
#include <cstdint>
#include <vector>
class Area;
class Block;
struct Shape;
struct MoveType;
struct FluidType;
class DramaticEvent
{
public:
	float probability() const = delete;
	void execute(Area& area) = delete;
protected:
	Block* getEntranceToArea(const Area& area, const MoveType& moveType) const;
	Block* findLocationForNear(const Shape& shape, const MoveType& moveType, const Block& block) const;
	std::vector<Block*> findOnEdgeAtSurface() const;
	bool blockIsConnectedToAtLeast(const Block& block, const MoveType& moveType, uint16_t count) const;
	Block* selectBlockForFlyingEntrance() const;
	Block* selectBlockForSwimmingEntrance(const FluidType& fluidType) const;
};
