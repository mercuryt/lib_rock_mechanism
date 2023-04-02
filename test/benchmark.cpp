#include "testShared.h"
#include <chrono>
#include <vector>
#include <iostream>
#include <sys/time.h>
#include <ctime>

std::chrono::milliseconds timeNow()
{
	return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

void fourFluids(uint32_t scale, uint32_t steps)
{
	uint32_t maxX = (scale * 2) + 2;
	uint32_t maxY = (scale * 2) + 2;
	uint32_t maxZ = (scale * 1) + 1;
	uint32_t halfMaxX = maxX / 2;
	uint32_t halfMaxY = maxY / 2;
	Area area(maxX, maxY, maxZ);
	s_step = 0;
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
	std::vector<FluidGroup*> newlySplit;
	// Water is at 0,0
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[halfMaxX - 1][halfMaxY - 1][maxZ - 1];		
	setFullFluidCuboid(area, water1, water2, s_water);
	// CO2 is at 0,1
	Block* CO2_1 = &area.m_blocks[1][halfMaxY][1];
	Block* CO2_2 = &area.m_blocks[halfMaxX - 1][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, CO2_1, CO2_2, s_CO2);
	// Lava is at 1,0
	Block* lava1 = &area.m_blocks[halfMaxX][1][1];
	Block* lava2 = &area.m_blocks[maxX - 2][halfMaxY - 1][maxZ - 1];
	setFullFluidCuboid(area, lava1, lava2, s_lava);
	// Mercury is at 1,1
	Block* mercury1 = &area.m_blocks[halfMaxX][halfMaxY][1];
	Block* mercury2 = &area.m_blocks[maxX - 2][maxY - 2][maxZ - 1];
	setFullFluidCuboid(area, mercury1, mercury2, s_mercury);
	s_step = 1;
	std::cout << "begin test four fluids at scale " << scale << std::endl;
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
		std::cout << std::to_string(s_step) << "step: " << std::to_string(timeStep) << "ms" << std::endl;
		s_step++;
		if(area.m_unstableFluidGroups.empty())
		{
			std::cout << "stabilized" << std::endl;
			break;
		}
	}
		auto timeTotal = (timeNow() - startTotal).count();
		std::cout << "total:" << std::to_string(timeTotal) << "ms" << std::endl;
		std::cout << "average:" << std::to_string(timeTotal / s_step) << "ms" << std::endl;
		std::cout << "longest:" << std::to_string(longestStepMs) << "ms" << std::endl;
}
int main()
{
	fourFluids(20,200);
}
