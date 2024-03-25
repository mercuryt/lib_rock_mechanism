#include "moveType.h"
const MoveType& MoveType::byName(std::string name)
{ 
	auto found = std::ranges::find(moveTypeDataStore, name, &MoveType::name);
	assert(found != moveTypeDataStore.end());
	return *found;
}
