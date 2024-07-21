#include "spaceFilling.h"
BlockIndex getIndexForPosition(Area& area, Point3D position)
{
	int modifier = 0;
	DistanceInBlocks shardSize = area.m_sizeX / 4;
	if(position.y && position.y < area.m_sizeY - 1)
		modifier = (((position.x / shardSize) % 2) | -1) * shardSize;
	return modifier + position.x + (position.y * area.m_sizeX) + (position.z * area.m_sizeX * area.m_sizeY);
}

Point3D getPositionForIndex(Area& area, BlockIndex index)
{
    Point3D position;

    // Calculate the z-coordinate
    position.z = index / (area.m_sizeX * area.m_sizeY);
    // Adjust the index for the next calculation
    index -= position.z * area.m_sizeX * area.m_sizeY;

    // Calculate the y-coordinate
    position.y = index / area.m_sizeX;
    // Adjust the index for the next calculation
    index -= position.y * area.m_sizeX;

    // Calculate the x-coordinate using the modifier
    DistanceInBlocks shardSize = area.m_sizeX / 4;
    int modifier = 0;
    if (position.y && position.y < area.m_sizeY - 1)
        modifier = (((position.x / shardSize) % 2) | -1) * shardSize;
    position.x = index - modifier;

    return position;
}
