#pragma once 

#include <vector>
#include <stack>
#include <unordered_set>

class Block;
class Actor;
class VisionRequest
{
public:
	Actor& m_actor;
	std::unordered_set<Actor*> m_actors;
	std::vector<const Block*> m_lineOfSight;
	std::unordered_set<const Block*> m_establishedAsHavingLineOfSight;

	static void readSteps(std::vector<VisionRequest>::iterator begin, std::vector<VisionRequest>::iterator end);
	
	VisionRequest(Actor& a);

	// Find actors in range.
	void readStep();
	// Run Actor::doVision on actors.
	void writeStep();
	// Some options for the line of sight function with various optomizations.
	bool hasLineOfSightUsingEstablishedAs(const Block& from, const Block& to);
	bool hasLineOfSightUsingVisionCuboidAndEstablishedAs(const Block& from, const Block& to);
	static bool hasLineOfSightBasic(const Block& from, const Block& to);
	bool hasLineOfSight(const Block& from, const Block& to);
};
