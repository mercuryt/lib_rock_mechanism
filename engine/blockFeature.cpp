#include "blockFeature.h"
#include "area/area.h"
#include "config.h"
// Name, hewn, blocks entrance, lockable, stand in, stand above
BlockFeatureType BlockFeatureType::floor{L"floor", false, false, false, true, false, false};
BlockFeatureType BlockFeatureType::door{L"door", false, false, true, false, false, false};
BlockFeatureType BlockFeatureType::flap{L"flap", false, false, false, false, false, false};
BlockFeatureType BlockFeatureType::hatch{L"hatch", false, false, true, true, false, false};
BlockFeatureType BlockFeatureType::stairs{L"stairs", true,  false, false, true, true, false};
BlockFeatureType BlockFeatureType::floodGate{L"floodGate", true, true, false, false, true, false};
BlockFeatureType BlockFeatureType::floorGrate{L"floorGrate", false, false, false, true, false, false};
BlockFeatureType BlockFeatureType::ramp{L"ramp", true, false, false, true, true, false};
BlockFeatureType BlockFeatureType::fortification{L"fortification", true, true, false, false, true, true};
