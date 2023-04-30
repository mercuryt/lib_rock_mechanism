#include "../testShared.h"
#include <chrono>
#include <vector>
#include <iostream>
#include <sys/time.h>
#include <ctime>

std::chrono::milliseconds timeNow()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

void rave(uint32_t scaleX, uint32_t scaleY, uint32_t scaleZ , uint32_t steps)
{
	uint32_t maxX = scaleX;
	uint32_t maxY = scaleY;
	uint32_t maxZ = scaleZ;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	area.visionCuboidsActivate();
	Cuboid<Block, Actor, Area> cuboid(area.m_blocks[scaleX - 1][scaleY - 1][scaleZ - 1], area.m_blocks[0][0][1]);
	std::list<Actor> actors;
	for(Block& block : cuboid)
	{
		block.addConstructedFeature(s_floor, s_glass);
		actors.emplace_back(&block, s_oneByOneFull, s_twoLegs);
		area.registerActor(actors.back());
	}
	s_step = 1;
	std::cout << "begin test glass rave at scale " << scaleX << "," << scaleY << "," << scaleZ << std::endl;
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
	rave(20, 20, 5, 100);
}
