#include "strongInteger.h"
#include "util.h"
template <class StrongInteger>
void StrongIntegerSet<StrongInteger>::removeDuplicatesAndValue(const StrongInteger& index) { assert(index.exists()); util::removeDuplicatesAndValue(data, index); }
