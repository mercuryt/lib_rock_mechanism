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

void fourFluids()
{
	uint32_t scale = 1;
	uint32_t maxX = scale * 10;
	uint32_t maxY = scale * 10;
	uint32_t maxZ = scale * 5;
	uint32_t halfMaxX = maxX / 2;
	uint32_t halfMaxY = maxY / 2;
	std::cout << "four fluids" << std::endl;
	Area area(maxX, maxY, maxZ);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, maxZ - 1, s_stone);
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
	assert(area.m_fluidGroups.size() == 4);
	FluidGroup* fgWater = water1->getFluidGroup(s_water);
	FluidGroup* fgCO2 = CO2_1->getFluidGroup(s_CO2);
	FluidGroup* fgLava = lava1->getFluidGroup(s_lava);
	FluidGroup* fgMercury = mercury1->getFluidGroup(s_mercury);
	std::cout << "setup completed" << std::endl;
	s_step = 1;
	while(not fgWater->m_stable or not fgCO2->m_stable or not fgLava->m_stable or not fgMercury->m_stable)
	{
		auto start = timeNow();
		area.readStep();
		s_pool.wait_for_tasks();
		area.writeStep();
		auto time = timeNow() - start;
		std::cout << std::to_string(s_step) << "step: " << std::to_string(time.count()) << "ms" << std::endl;
		assert(area.m_fluidGroups.size() == 4);
		s_step++;
	}
}
int main()
{
	fourFluids();
}
