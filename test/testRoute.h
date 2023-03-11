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
	CHECK(routeRequest.m_moveCostsToCache.at(&origin).size() == 4);
	CHECK(routeRequest.m_result.size() == 8);
	routeRequest.writeStep();
	CHECK(actor.m_route->size() == 8);
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
	CHECK(routeRequest.m_moveCostsToCache.at(&origin).size() == 4);
	CHECK(routeRequest.m_result.size() == 10);
	routeRequest.writeStep();
	CHECK(actor.m_route->size() == 10);
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
	Block& block1 = area.m_blocks[3][4][1];
	Block& destination = area.m_blocks[4][4][1];
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
	Block& block3 = area.m_blocks[2][4][1];
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
