#include "blockIndices.h"
#include "simulation.h"
BlockIndex BlockIndices::random(Simulation& simulation) const
{
	return simulation.m_random.getInVector(data);
}
