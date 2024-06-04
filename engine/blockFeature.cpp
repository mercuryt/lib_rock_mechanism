#include "blockFeature.h"
#include "area.h"
#include "config.h"
// Name, hewn, blocks entrance, lockable, stand in, stand above
BlockFeatureType BlockFeatureType::floor{"floor", false, false, false, true, false, false};
BlockFeatureType BlockFeatureType::door{"door", false, false, true, false, false, false};
BlockFeatureType BlockFeatureType::flap{"flap", false, false, false, false, false, false};
BlockFeatureType BlockFeatureType::hatch{"hatch", false, false, true, true, false, false};
BlockFeatureType BlockFeatureType::stairs{"stairs", true,  false, false, true, true, false};
BlockFeatureType BlockFeatureType::floodGate{"floodGate", true, true, false, false, true, false};
BlockFeatureType BlockFeatureType::floorGrate{"floorGrate", false, false, false, true, false, false};
BlockFeatureType BlockFeatureType::ramp{"ramp", true, false, false, true, true, false};
BlockFeatureType BlockFeatureType::fortification{"fortification", true, true, false, false, true, true};
