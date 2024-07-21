#include "../../lib/doctest.h"
#include "../../engine/reservable.h"
#include "../../engine/faction.h"
#include <memory>

struct TestReservationDishonorCallback1 final : public DishonorCallback
{
	bool& fired;
	TestReservationDishonorCallback1(bool& f) : fired(f) { }
	void execute(uint32_t o, uint32_t n)
	{
			REQUIRE(o == 1);
			REQUIRE(n == 0);
			fired = true; 
	}
	Json toJson() const { return {}; }
};
struct TestReservationDishonorCallback2 final : public DishonorCallback
{
	bool& fired;
	TestReservationDishonorCallback2(bool& f) : fired(f) { }
	void execute(uint32_t o, uint32_t n)
	{
			REQUIRE(o == 2);
			REQUIRE(n == 1);
			fired = true;
	}
	Json toJson() const { return {}; }
};
TEST_CASE("reservations")
{
	Faction faction1(L"test faction1");
	Faction faction2(L"test faction2");
	SUBCASE("basic")
	{
		Reservable reservable(1);
		CanReserve canReserve(faction1);
		REQUIRE(!reservable.isFullyReserved(faction1));
		REQUIRE(!reservable.isFullyReserved(faction2));
		reservable.reserveFor(canReserve, 1);
		REQUIRE(reservable.isFullyReserved(faction1));
		REQUIRE(!reservable.isFullyReserved(faction2));
		reservable.clearReservationFor(canReserve);
		REQUIRE(!reservable.isFullyReserved(faction1));
		REQUIRE(!reservable.isFullyReserved(faction2));
		reservable.setMaxReservations(2);
		reservable.reserveFor(canReserve, 1);
		REQUIRE(!reservable.isFullyReserved(faction1));
		REQUIRE(!reservable.isFullyReserved(faction2));
	}
	SUBCASE("unreserve on destroy canReserve")
	{
		Reservable reservable(1);
		{
			CanReserve canReserve(faction1);
			reservable.reserveFor(canReserve, 1);
			REQUIRE(reservable.isFullyReserved(faction1));
		}
		REQUIRE(!reservable.isFullyReserved(faction1));
		REQUIRE(reservable.getUnreservedCount(faction1) == 1);
	}
	SUBCASE("clear reservation from canReserve when reservable is destroyed")
	{
		CanReserve canReserve(faction1);
		{
			Reservable reservable(1);
			reservable.reserveFor(canReserve, 1);
			REQUIRE(reservable.isFullyReserved(faction1));
			REQUIRE(canReserve.hasReservationWith(reservable));
		}
		REQUIRE(!canReserve.hasReservations());
	}
	SUBCASE("mutiple reservable")
	{
		Reservable reservable(2);
		CanReserve canReserve(faction1);
		reservable.reserveFor(canReserve, 1);
		REQUIRE(!reservable.isFullyReserved(faction1));
		REQUIRE(reservable.getUnreservedCount(faction1) == 1);
		reservable.reserveFor(canReserve, 1);
		REQUIRE(reservable.isFullyReserved(faction1));
	}
	SUBCASE("dishonor callback clear all")
	{
		bool fired = false;
		std::unique_ptr<DishonorCallback> callback = std::make_unique<TestReservationDishonorCallback1>(fired);
		Reservable reservable(1);
		CanReserve canReserve(faction1);
		reservable.reserveFor(canReserve, 1, std::move(callback));
		reservable.clearAll();
		REQUIRE(fired);
	}
	SUBCASE("dishonor callback reduce max reservable")
	{
		bool fired = false;
		std::unique_ptr<DishonorCallback> callback = std::make_unique<TestReservationDishonorCallback2>(fired);
		Reservable reservable(2);
		CanReserve canReserve(faction1);
		reservable.reserveFor(canReserve, 2, std::move(callback));
		reservable.setMaxReservations(1);
		REQUIRE(fired);
	}
}
