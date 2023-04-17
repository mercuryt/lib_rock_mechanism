#include "../testShared.h"
#include <chrono>
#include <vector>
#include <iostream>
#include <sys/time.h>
#include <ctime>
#include <random>

std::chrono::milliseconds timeNow()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

static std::mt19937 getRandom;
Block& getStart(Area& area)
{
	Block* block = nullptr;
	while(block == nullptr || !block->canEnterEver() || !block->m_actors.empty())
		block = area.m_blocks[getRandom()%area.m_sizeX][getRandom()%area.m_sizeY][getRandom()%area.m_sizeZ];
	return block;
}
Block& getDestination(Area& area)
{
	Block* block = nullptr;
	while(block == nullptr || !block->canEnterEver())
		block = area.m_blocks[getRandom()%area.m_sizeX][getRandom()%area.m_sizeY][getRandom()%area.m_sizeZ];
	return block;
}
void wander(uint32_t scale, uint32_t actors, uint32_t steps)
{
	Area area(scale, scale, 2);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Cuboid cuboid(area.m_blocks[scale - 2][scale - 2][scale - 2], area.m_blocks[1][1][1]);
	std::list<Actor> actors;
	for(Block& block : cuboid)
	{
		if(block.m_x % 2 == 1)
			continue;
		if(block.m_y % 2 == 1 || getRandom() % 5 == 0)
			block.setSolid(s_stone);
	}
	for(uint32_t i = 0; i < actors; ++i)
	{
		actors.emplace_back(&getStart(area), s_oneByOneFull, s_twoLegs);
		area.registerActor(actors.back());
		actors.back().setDestination(getDestination(area));
	}
	s_step = 1;
	area.visionCuboidsActivate();
	std::cout << "begin test wander at scale " << scale << std::endl;
	std::cout << actors.size() << " actors." << std::endl;
	auto startTotal = timeNow();
	uint32_t longestStepMs = 0;
	while(s_step < steps)
	{
		auto startStep = timeNow();
		area.readStep();
		area.writeStep();
		auto timeStep = (timeNow() - startStep).count();
		if(timeStep > longestStepMs)
			longestStepMs = timeStep;
		s_step++;
	}
	auto timeTotal = (timeNow() - startTotal).count();
	std::cout << "total:" << std::to_string(timeTotal) << "ms" << std::endl;
	std::cout << "average:" << std::to_string(timeTotal / s_step) << "ms" << std::endl;
	std::cout << "longest:" << std::to_string(longestStepMs) << "ms" << std::endl;
}
int main()
{
	wander(100, 20, 1500);
}
