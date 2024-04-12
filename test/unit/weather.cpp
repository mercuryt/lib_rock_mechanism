#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/threadedTask.h"
#include "config.h"
#include <iterator>
#include <iostream>

TEST_CASE("weather")
{
	static const MaterialType& marble = MaterialType::byName("marble");
	static const FluidType& water = FluidType::byName("water");
	Simulation simulation{L"", 1};
	Area& area = simulation.createArea(5, 5, 5);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	area.m_hasRain.start(100, Config::stepsPerMinute);
	for(uint i = 0; i < Config::stepsPerMinute; ++i)
	{
		simulation.doStep();
		if(simulation.m_step % Config::stepsPerMinute == 0)
		{
			std::cout << "." << std::flush;
		}

	}
	REQUIRE(area.getBlock(1,1,1).volumeOfFluidTypeContains(water));
}
