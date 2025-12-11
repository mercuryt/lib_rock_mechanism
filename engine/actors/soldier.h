#pragma once
#include "../squad.h"

struct SoldierData
{
	CuboidSet maliceZone;
	SquadId squad;
	bool isDrafted;
	static SoldierData create() { SoldierData output; output.isDrafted = false; return output; }
};