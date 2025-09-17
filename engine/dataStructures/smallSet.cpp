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

template struct SmallSet<ActorIndex>;
template struct SmallSet<ActorOrItemIndex>;
template struct SmallSet<ActorOrItemReference>;
template struct SmallSet<ActorReference>;
template struct SmallSet<CraftJob*>;
template struct SmallSet<Cuboid>;
template struct SmallSet<ItemIndex>;
template struct SmallSet<ItemReference>;
template struct SmallSet<std::pair<OffsetCuboid, CollisionVolume>>;
template struct SmallSet<OffsetCuboid>;
template struct SmallSet<PlantIndex>;
template struct SmallSet<Point3D>;
template struct SmallSet<Offset3D>;
template struct SmallSet<Project*>;
template struct SmallSet<uint>;
template struct SmallSet<RTreeBoolean::Index>;
template struct SmallSet<std::pair<Cuboid, uint>>;
template struct SmallSet<std::pair<OffsetCuboid, uint>>;
template struct SmallSet<RTreeNodeIndex>;