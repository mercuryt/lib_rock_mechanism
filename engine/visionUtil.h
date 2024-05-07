#pragma once
class Block;
namespace visionUtil
{
	bool hasLineOfSightUsingVisionCuboid(const Block& from, const Block& to);
	bool hasLineOfSightBasic(const Block& from, const Block& to);
}
