#include "blockFeature.h"
#include "area/area.h"
#include "config.h"
// Name, hewn, blocks entrance, lockable, stand in, stand above, blocks multi tile shapes if not at zero z offset.
BlockFeatureType BlockFeatureType::floor{"floor", false, false, false, true, false, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::door{"door", false, false, true, false, false, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::flap{"flap", false, false, false, false, false, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::hatch{"hatch", false, false, true, true, false, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::stairs{"stairs", true,  false, false, true, true, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::floodGate{"floodGate", true, true, false, false, true, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::floorGrate{"floorGrate", false, false, false, true, false, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::ramp{"ramp", true, false, false, true, true, false, true, Force::create(0), 1};
BlockFeatureType BlockFeatureType::fortification{"fortification", true, true, false, false, true, true, true, Force::create(0), 1};
