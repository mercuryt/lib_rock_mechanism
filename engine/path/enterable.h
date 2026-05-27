/*
	A thin wrapper for rtree boolean to allow space to prevent pathable cuboids from mergeing when a zero thickness partition exists between them.
*/
#pragma once
#include "../dataStructures/rtreeBoolean.h"
#include "../space/space.h"

class Enterable final : public RTreeBoolean
{
	const Space& m_space;
	bool canMerge(const Cuboid a, const Cuboid b) const
	{
		return !m_space.move_partitionExistsBetween(a, b);
	}
public:
	Enterable(const Space& space) : RTreeBoolean(), m_space(space) { }
};