#pragma once

#include <string>
#include <vector>
#include <cassert>
#include <algorithm>
struct Hit;
struct BodyPartType;
struct WoundType
{
	const std::string name;
	virtual uint32_t getStepsTillHealed(const Hit& hit, const BodyPartType& bodyPartType) const = 0;
	virtual uint32_t getBleedVolumeRate(const Hit& hit, const BodyPartType& bodyPartType) const = 0;
	virtual uint32_t getPercentTemporaryImpairment(const Hit& hit, const BodyPartType& bodyPartType) const = 0;
	virtual uint32_t getPercentPermanentImpairment(const Hit& hit, const BodyPartType& bodyPartType) const = 0;
	// Infastructure.
	bool operator==(const WoundType& woundType){ return this == &woundType; }
	inline static std::vector<WoundType> data;
	static const WoundType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &WoundType::name);
		assert(found != data.end());
		return *found;
	}
};
