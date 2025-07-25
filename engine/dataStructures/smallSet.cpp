#include "smallSet.h"
#include "../geometry/point3D.h"
#include "../geometry/cuboid.h"
#include "../definitions/shape.h"
#include "../actorOrItemIndex.h"

class Project;

template class SmallSet<Point3D>;
template class SmallSet<Cuboid>;
template class SmallSet<ActorIndex>;
template class SmallSet<ItemIndex>;
template class SmallSet<ActorOrItemIndex>;
template class SmallSet<PlantIndex>;
template class SmallSet<OffsetAndVolume>;
template class SmallSet<uint>;