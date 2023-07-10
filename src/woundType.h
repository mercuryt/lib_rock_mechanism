#pragma once

#include <string>
struct Hit;
struct BodyPartType;
struct WoundType
{
	const std::string name;
	virtual uint32_t getStepsTillHealed(Hit& hit, BodyPartType& bodyPartType) = 0;
	virtual uint32_t getBleedVolumeRate(Hit& hit, BodyPartType& bodyPartType) = 0;
	virtual uint32_t getPercentTemporaryImpairment(Hit& hit, BodyPartType& bodyPartType) = 0;
	virtual uint32_t getPercentPermanentImpairment(Hit& hit, BodyPartType& bodyPartType) = 0;
	// Infastructure.
	bool operator==(const WoundType& woundType){ return this == &woundType; }
	static std::vector<WoundType> data;
	static const WoundType& byName(const std::string name)
	{
		auto found = std::ranges::find(data, name, &WoundType::name);
		assert(found != data.end());
		return *found;
	}
};
