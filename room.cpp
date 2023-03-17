static bool Room::cornersAreValid(Block* cornerA, Block* cornerB)
{
	if(!VisionRequest::hasLineOfSight(cornerA, cornerB))
		return false;
	Block* cornerC = cornerA->m_area->m_blocks[cornerA->m_x][cornerB->m_y][cornerA->m_z];
	Block* cornerD = cornerA->m_area->m_blocks[cornerA->m_x][cornerB->m_y][cornerB->m_z];
	if(!VisionRequest::hasLineOfSight(cornerC, cornerD))
		return false;
	Block* cornerE = cornerA->m_area->m_blocks[cornerB->m_x][cornerA->m_y][cornerA->m_z];
	Block* cornerF = cornerA->m_area->m_blocks[cornerB->m_x][cornerA->m_y][cornerB->m_z];
	if(!VisionRequest::hasLineOfSight(cornerE, cornerF))
		return false;
	return true;
}
Room::Room(Block* cornerA, Block* cornerB, bool useForVision) : m_useForVision(useForVision)
{
	assert(cornerA != cornerB);
	uint32_t maxX = std::max(cornerA->m_x, cornerB->m_x);
	uint32_t minX = std::min(cornerA->m_x, cornerB->m_x);
	uint32_t maxY = std::max(cornerA->m_y, cornerB->m_y);
	uint32_t minY = std::min(cornerA->m_y, cornerB->m_y);
	uint32_t maxZ = std::max(cornerA->m_z, cornerB->m_z);
	uint32_t minZ = std::min(cornerA->m_z, cornerB->m_z);
	for(uint32_t x = minX; x != maxX; ++x)
		for(uint32_t y = minY; y != maxY; ++y)
			for(uint32_t z = minZ; z != maxZ; ++z)
			{
				Block* block = cornerA->m_area->m_blocks[x][y][z]
				m_blocks.push_back(block);
				block.m_room = this;
			}
	m_largestDimension = std::max({maxX-minX, maxY-minY, maxZ-minZ});
}
