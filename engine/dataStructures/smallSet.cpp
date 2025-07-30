#include "smallSet.hpp"
#include "rtreeBoolean.h"
#include "../geometry/point3D.h"
#include "../geometry/cuboid.h"
#include "../geometry/offsetCuboid.h"
#include "../definitions/shape.h"
#include "../actorOrItemIndex.h"
#include "../reference.h"

class Project;
class CraftJob;

template class SmallSet<ActorIndex>;
template class SmallSet<ActorOrItemIndex>;
template class SmallSet<ActorOrItemReference>;
template class SmallSet<ActorReference>;
template class SmallSet<CraftJob*>;
template class SmallSet<Cuboid>;
template class SmallSet<ItemIndex>;
template class SmallSet<ItemReference>;
template class SmallSet<OffsetAndVolume>;
template class SmallSet<OffsetCuboid>;
template class SmallSet<PlantIndex>;
template class SmallSet<Point3D>;
template class SmallSet<Project*>;
template class SmallSet<uint>;
template class SmallSet<RTreeBoolean::Index>;
template class SmallSet<std::pair<Cuboid, uint>>;