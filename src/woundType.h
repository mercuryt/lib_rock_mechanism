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
};
