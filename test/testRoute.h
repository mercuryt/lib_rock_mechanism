TEST_CASE("Route through open space")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[3][3][1];
	Block& destination = area.m_blocks[7][7][1];
	Actor actor(&origin, s_oneByOneFull, s_twoLegs);
	actor.setDestination(&destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(!routeRequest.m_moveCostsToCache.empty());
	CHECK(routeRequest.m_moveCostsToCache.at(&origin).size() == 8);
	CHECK(routeRequest.m_result.size() == 4);
	routeRequest.writeStep();
	CHECK(actor.m_route->size() == 4);
}
TEST_CASE("Route around walls")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[3][3][1];
	Block& block1 = area.m_blocks[5][3][1];
	Block& block2 = area.m_blocks[5][4][1];
	Block& block3 = area.m_blocks[5][5][1];
	Block& block4 = area.m_blocks[5][6][1];
	Block& block5 = area.m_blocks[5][7][1];
	Block& destination = area.m_blocks[7][7][1];
	block1.setSolid(s_stone);
	block2.setSolid(s_stone);
	block3.setSolid(s_stone);
	block4.setSolid(s_stone);
	block5.setSolid(s_stone);
	Actor actor(&origin, s_oneByOneFull, s_twoLegs);
	actor.setDestination(&destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(!routeRequest.m_moveCostsToCache.empty());
	CHECK(routeRequest.m_moveCostsToCache.at(&origin).size() == 8);
	CHECK(routeRequest.m_result.size() == 7);
	routeRequest.writeStep();
	CHECK(actor.m_route->size() == 7);
}
TEST_CASE("No route found")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[0][0][1];
	Block& block1 = area.m_blocks[2][0][1];
	Block& block2 = area.m_blocks[2][1][1];
	Block& block3 = area.m_blocks[2][2][1];
	Block& block4 = area.m_blocks[1][2][1];
	Block& block5 = area.m_blocks[0][2][1];
	Block& destination = area.m_blocks[7][7][1];
	block1.setSolid(s_stone);
	block2.setSolid(s_stone);
	block3.setSolid(s_stone);
	block4.setSolid(s_stone);
	block5.setSolid(s_stone);
	Actor actor(&origin, s_oneByOneFull, s_twoLegs);
	actor.setDestination(&destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.empty());
	routeRequest.writeStep();
	CHECK(actor.m_route == nullptr);
}
TEST_CASE("Walk")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[3][3][1];
	Block& block1 = area.m_blocks[4][4][1];
	Block& destination = area.m_blocks[5][5][1];
	Actor actor(&origin, s_oneByOneFull, s_twoLegs);
	actor.setDestination(&destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	routeRequest.writeStep();
	CHECK(area.m_scheduledEvents.size() == 1);
	uint32_t scheduledStep = area.m_scheduledEvents.begin()->first;
	s_step = scheduledStep;
	area.executeScheduledEvents(scheduledStep);
	CHECK(actor.m_location == &block1);
	CHECK(area.m_scheduledEvents.size() == 1);
	scheduledStep = area.m_scheduledEvents.begin()->first;
	s_step = scheduledStep;
	area.executeScheduledEvents(scheduledStep);
	CHECK(actor.m_location == &destination);
	CHECK(area.m_scheduledEvents.size() == 0);
	CHECK(actor.m_route == nullptr);
	CHECK(actor.m_destination == nullptr);
}
TEST_CASE("Repath when route is blocked")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[3][3][1];
	Block& block1 = area.m_blocks[3][4][1];
	Block& block2 = area.m_blocks[3][5][1];
	Block& block3 = area.m_blocks[4][5][1];
	Block& destination = area.m_blocks[3][6][1];
	Actor actor(&origin, s_oneByOneFull, s_twoLegs);
	actor.setDestination(&destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	routeRequest.writeStep();
	area.m_routeRequestQueue.clear();
	CHECK(area.m_scheduledEvents.size() == 1);
	// Step 1.
	uint32_t scheduledStep = area.m_scheduledEvents.begin()->first;
	s_step = scheduledStep;
	area.executeScheduledEvents(scheduledStep);
	CHECK(actor.m_location == &block1);
	CHECK(area.m_scheduledEvents.size() == 1);
	block2.setSolid(s_stone);
	CHECK(origin.m_routeCacheVersion != area.m_routeCacheVersion);
	// Step 2.
	CHECK(area.m_scheduledEvents.size() == 1);
	scheduledStep = area.m_scheduledEvents.begin()->first;
	s_step = scheduledStep;
	area.executeScheduledEvents(scheduledStep);
	CHECK(actor.m_location == &block1);
	CHECK(area.m_scheduledEvents.size() == 0);
	CHECK(area.m_routeRequestQueue.size() == 1);
	// Step 3.
	routeRequest = area.m_routeRequestQueue.back();
	routeRequest.readStep();
	routeRequest.writeStep();
	area.m_routeRequestQueue.clear();
	CHECK(area.m_scheduledEvents.size() == 1);
	scheduledStep = area.m_scheduledEvents.begin()->first;
	s_step = scheduledStep;
	area.executeScheduledEvents(scheduledStep);
	CHECK(actor.m_location == &block3);
}
TEST_CASE("Walk multi-block creature")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[3][3][1];
	Block& block1 = area.m_blocks[4][4][1];
	Block& destination = area.m_blocks[5][5][1];
	Actor actor(&origin, s_twoByTwoFull, s_twoLegs);
	actor.setDestination(&destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.size() == 2);
	routeRequest.writeStep();
	CHECK(area.m_scheduledEvents.size() == 1);
	uint32_t scheduledStep = area.m_scheduledEvents.begin()->first;
	s_step = scheduledStep;
	area.executeScheduledEvents(scheduledStep);
	CHECK(actor.m_location == &block1);
	CHECK(area.m_scheduledEvents.size() == 1);
	scheduledStep = area.m_scheduledEvents.begin()->first;
	s_step = scheduledStep;
	area.executeScheduledEvents(scheduledStep);
	CHECK(actor.m_location == &destination);
	CHECK(area.m_scheduledEvents.size() == 0);
	CHECK(actor.m_route == nullptr);
	CHECK(actor.m_destination == nullptr);
}
TEST_CASE("two by two creature cannot path through one block gap.")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block& origin = area.m_blocks[2][5][1];
	Block& destination = area.m_blocks[8][5][1];
	area.m_blocks[1][5][1].setSolid(s_stone);
	area.m_blocks[3][5][1].setSolid(s_stone);
	area.m_blocks[5][5][1].setSolid(s_stone);
	area.m_blocks[7][5][1].setSolid(s_stone);
	area.m_blocks[9][5][1].setSolid(s_stone);
	Actor actor(&origin, s_twoByTwoFull, s_twoLegs);
	actor.setDestination(&destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.empty());
}
TEST_CASE("non-flying can't fly path")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, 9, s_stone);
	Block* origin = &area.m_blocks[1][1][1];
	Block* destination = &area.m_blocks[8][8][8];		
	Block* ledge = &area.m_blocks[8][8][7];
	ledge->setSolid(s_stone);
	Actor actor(origin, s_oneByOneFull, s_fourLegs);
	actor.setDestination(destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.size() == 0);
}
TEST_CASE("flying path")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, 9, s_stone);
	Block* origin = &area.m_blocks[1][1][1];
	Block* destination = &area.m_blocks[8][8][8];		
	Actor actor(origin, s_oneByOneFull, s_flying);
	actor.setDestination(destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.size() == 7);
}
TEST_CASE("swimming path")
{
	Area area(10,10,10);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, 9, s_stone);
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[8][8][8];		
	setFullFluidCuboid(water1, water2, s_water);
	Actor actor(water1, s_oneByOneFull, s_swimmingInWater);
	actor.setDestination(water2);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.size() == 7);
}
TEST_CASE("swimming path blocked")
{
	Area area(5,5,2);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	setSolidWalls(area, 1, s_stone);
	Block* origin = &area.m_blocks[2][1][1];
	Block* destination = &area.m_blocks[2][3][1];		
	area.m_blocks[1][2][1].setSolid(s_stone);
	area.m_blocks[2][2][1].setSolid(s_stone);
	area.m_blocks[3][2][1].setSolid(s_stone);
	Block* water1 = &area.m_blocks[1][1][1];
	Block* water2 = &area.m_blocks[3][1][1];
	setFullFluidCuboid(water1, water2, s_water);
	Block* water3 = &area.m_blocks[1][3][1];
	Block* water4 = &area.m_blocks[3][3][1];
	setFullFluidCuboid(water3, water4, s_water);
	Actor actor(origin, s_oneByOneFull, s_swimmingInWater);
	actor.setDestination(destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.size() == 0);
}
TEST_CASE("walking path blocked by water")
{
	Area area(5,5,3);
	registerTypes();
	setSolidLayers(area, 0, 1, s_stone);
	setSolidWalls(area, 2, s_stone);
	Block* origin = &area.m_blocks[2][1][2];
	Block* destination = &area.m_blocks[2][3][2];		
	area.m_blocks[1][2][1].setNotSolid();
	area.m_blocks[2][2][1].setNotSolid();
	area.m_blocks[3][2][1].setNotSolid();
	Block* water1 = &area.m_blocks[1][2][1];
	Block* water2 = &area.m_blocks[3][2][1];
	setFullFluidCuboid(water1, water2, s_water);
	Actor actor(origin, s_oneByOneFull, s_twoLegs);
	actor.setDestination(destination);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.size() == 0);
	actor.m_moveType = s_twoLegsAndSwimmingInWater;
	RouteRequest routeRequest2(&actor);
	routeRequest2.readStep();
	CHECK(routeRequest2.m_result.size() != 0);
}
TEST_CASE("stairs")
{
	Area area(5,5,5);
	registerTypes();
	setSolidLayer(area, 0, s_stone);
	Block* origin = &area.m_blocks[1][1][1];
	area.m_blocks[2][2][1].addConstructedFeature(s_upDownStairs, s_stone);
	area.m_blocks[2][2][2].addConstructedFeature(s_upDownStairs, s_stone);
	area.m_blocks[2][2][3].addConstructedFeature(s_upDownStairs, s_stone);
	Actor actor(origin, s_oneByOneFull, s_twoLegs);
	actor.setDestination(&area.m_blocks[2][2][1]);
	RouteRequest routeRequest(&actor);
	routeRequest.readStep();
	CHECK(routeRequest.m_result.size() != 0);
}
