/*
 * A room. Can be used for optimizing vision indoors. Can be extended for other purposes like bed rooms and dining rooms.
 */
#include <cstdint>
class Block;
class VisionRequest;
class Room
{
	std::vector<Block*> m_blocks;
	bool m_useForVision;
	uint32_t m_largestDimension;

	static bool cornersAreValid(Block* a, Block* b);

	Room(Block* cornerA, Block* cornerB, bool useForVision);
}
