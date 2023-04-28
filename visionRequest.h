#pragma once 

#include <vector>
#include <stack>
#include <unordered_set>

template<class DerivedBlock, class DerivedActor, class DerivedArea>
class VisionRequest
{
public:
	DerivedActor& m_actor;
	std::unordered_set<DerivedActor*> m_actors;
	std::vector<const DerivedBlock*> m_lineOfSight;
	std::unordered_set<const DerivedBlock*> m_establishedAsHavingLineOfSight;

	static void readSteps(std::vector<VisionRequest<DerivedBlock, DerivedActor, DerivedArea>>::iterator begin, std::vector<VisionRequest<DerivedBlock, DerivedActor, DerivedArea>>::iterator end);
	
	VisionRequest(DerivedActor& a);

	// Find actors in range.
	void readStep();
	// Run DerivedActor::doVision on actors.
	void writeStep();
	// Some options for the line of sight function with various optomizations.
	bool hasLineOfSightUsingEstablishedAs(const DerivedBlock& from, const DerivedBlock& to);
	bool hasLineOfSightUsingVisionCuboidAndEstablishedAs(const DerivedBlock& from, const DerivedBlock& to);
	static bool hasLineOfSightUsingVisionCuboid(const DerivedBlock& from, const DerivedBlock& to);
	static bool hasLineOfSightBasic(const DerivedBlock& from, const DerivedBlock& to);
	bool hasLineOfSight(const DerivedBlock& from, const DerivedBlock& to);
};
