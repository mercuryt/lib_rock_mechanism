#include "../../lib/doctest.h"
#include "../../src/reservable.h"
#include "../../src/faction.h"
TEST_CASE("reservations")
{
	const Faction faction1(L"test faction1");
	const Faction faction2(L"test faction2");
	Reservable reservable(1);
	CanReserve canReserve(&faction1);
	REQUIRE(!reservable.isFullyReserved(&faction1));
	REQUIRE(!reservable.isFullyReserved(&faction2));
	reservable.reserveFor(canReserve, 1);
	REQUIRE(reservable.isFullyReserved(&faction1));
	REQUIRE(!reservable.isFullyReserved(&faction2));
	reservable.clearReservationFor(canReserve);
	REQUIRE(!reservable.isFullyReserved(&faction1));
	REQUIRE(!reservable.isFullyReserved(&faction2));
	reservable.setMaxReservations(2);
	reservable.reserveFor(canReserve, 1);
	REQUIRE(!reservable.isFullyReserved(&faction1));
	REQUIRE(!reservable.isFullyReserved(&faction2));
}
