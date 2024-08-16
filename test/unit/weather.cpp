#include "../../lib/doctest.h"
#include "../../engine/area.h"
#include "../../engine/areaBuilderUtil.h"
#include "../../engine/simulation.h"
#include "../../engine/simulation/hasAreas.h"
#include "../../engine/threadedTask.h"
#include "../../engine/actors/actors.h"
#include "../../engine/items/items.h"
#include "../../engine/plants.h"
#include "config.h"
#include <iterator>
#include <iostream>

TEST_CASE("weather")
{
	static MaterialTypeId marble = MaterialType::byName("marble");
	static FluidTypeId water = FluidType::byName("water");
	Simulation simulation{L"", Step::create(1)};
	Area& area = simulation.m_hasAreas->createArea(5, 5, 5);
	areaBuilderUtil::setSolidLayer(area, 0, marble);
	area.m_hasRain.start(Percent::create(100), Config::stepsPerMinute);
	for(uint i = 0; i < Config::stepsPerMinute; ++i)
	{
		simulation.doStep();
		if(simulation.m_step.modulusIsZero(Config::stepsPerMinute))
		{
			std::cout << "." << std::flush;
		}

	}
	Blocks& blocks = area.getBlocks();
	REQUIRE(blocks.fluid_volumeOfTypeContains(blocks.getIndex_i(1,1,1), water) != 0);
}
