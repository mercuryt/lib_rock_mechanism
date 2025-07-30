#include "limitedSet.hpp"
#include "../geometry/cuboid.h"

template class LimitedSet<Point3D, 4>;
template class LimitedSet<Point3D, 6>;
template class LimitedSet<Point3D, 8>;
template class LimitedSet<Point3D, 26>;
template class LimitedSet<Cuboid, 6>;