#pragma once 

#include <vector>
#include <stack>
#include <unordered_set>

class VisionRequest
{
public:
	ACTOR& m_actor;
	std::unordered_set<ACTOR*> m_actors;
	std::vector<const BLOCK*> m_lineOfSight;
	std::unordered_set<const BLOCK*> m_establishedAsHavingLineOfSight;

	static void readSteps(std::vector<VisionRequest>::iterator begin, std::vector<VisionRequest>::iterator end);
	
	VisionRequest(ACTOR& a);

	// Find actors in range.
	void readStep();
	// Run ACTOR::doVision on actors.
	void writeStep();
	// Some options for the line of sight function with various optomizations.
	bool hasLineOfSightUsingEstablishedAs(const BLOCK& from, const BLOCK& to);
	bool hasLineOfSightUsingVisionCuboidAndEstablishedAs(const BLOCK& from, const BLOCK& to);
	static bool hasLineOfSightUsingVisionCuboid(const BLOCK& from, const BLOCK& to);
	static bool hasLineOfSightBasic(const BLOCK& from, const BLOCK& to);
	bool hasLineOfSight(const BLOCK& from, const BLOCK& to);
};
