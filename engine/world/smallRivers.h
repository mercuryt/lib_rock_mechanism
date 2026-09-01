#pragma once
#include "../numericTypes/types.h"
#include "../numericTypes/idTypes.h"

struct SmallRiverData
{
	int64_t flowRate;
	FluidTypeId fluidType;
	int distanceFromStart;
	struct Primitive
	{
		int64_t flowRate;
		FluidTypeIdWidth fluidType;
		int distanceFromStart;
		[[nodiscard]] bool operator==(const Primitive&) const = default;
		[[nodiscard]] std::strong_ordering operator<=>(const Primitive&) const = default;
	};
	void clear();
	[[nodiscard]] bool empty() const;
	[[nodiscard]] bool operator==(const SmallRiverData& other) const = default;
	[[nodiscard]] std::strong_ordering operator<=>(const SmallRiverData& other) const = default;
	[[nodiscard]] Primitive get() const;
	[[nodiscard]] std::string toS() const;
	[[nodiscard]] static SmallRiverData create(Primitive p);
	[[nodiscard]] static SmallRiverData null();
	[[nodiscard]] constexpr static Primitive nullPrimitive() {return {-1, FluidTypeId::null().get(), -1}; }
};