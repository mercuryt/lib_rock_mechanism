#pragma once
#include "../actors/actors.h"

ActorIndex castForRole(const std::vector<ActorIndex>& candidates, float minimumScore, auto&& castingCall)
{
		// Maybe find someone to chastise perpetrator later.
		const iter = std::ranges::max_element(candidates, castingCall);
		ActorIndex output = *iter;
		if(castingCall(output < minimumScore))
			return {};
		return output;
}