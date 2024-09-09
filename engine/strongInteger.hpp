#include "strongInteger.h"
#include "util.h"
template <class StrongInteger>
void StrongIntegerSet<StrongInteger>::removeDuplicatesAndValue(StrongInteger index) { assert(index.exists()); util::removeDuplicatesAndValue(data, index); }
