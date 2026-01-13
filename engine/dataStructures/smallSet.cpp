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

template struct std::vector<ActorIndex>;
template struct std::vector<ActorOrItemIndex>;
template struct std::vector<ActorOrItemReference>;
template struct std::vector<ActorReference>;
template struct std::vector<CraftJob*>;
template struct std::vector<Cuboid>;
template struct std::vector<ItemIndex>;
template struct std::vector<ItemReference>;
template struct std::vector<std::pair<OffsetCuboid, CollisionVolume>>;
template struct std::vector<OffsetCuboid>;
template struct std::vector<PlantIndex>;
template struct std::vector<Point3D>;
template struct std::vector<Offset3D>;
template struct std::vector<Project*>;
template struct std::vector<int32_t>;
template struct std::vector<int16_t>;
template struct std::vector<std::pair<Cuboid, int32_t>>;
template struct std::vector<std::pair<OffsetCuboid, int32_t>>;
template struct std::vector<RTreeNodeIndex>;

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
template struct SmallSet<int32_t>;
template struct SmallSet<int16_t>;
template struct SmallSet<std::pair<Cuboid, int32_t>>;
template struct SmallSet<std::pair<OffsetCuboid, int32_t>>;
template struct SmallSet<RTreeArrayIndex>;