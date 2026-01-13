#pragma once
#include "../squad.h"

struct SoldierData
{
	CuboidSet maliceZone;
	SquadIndex squad;
	bool isDrafted;
	static SoldierData create() { SoldierData output; output.isDrafted = false; return output; }
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SoldierData, maliceZone, squad, isDrafted);
};