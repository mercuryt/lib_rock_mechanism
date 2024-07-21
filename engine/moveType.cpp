#include "moveType.h"
const MoveType& MoveType::byName(std::string name)
{ 
	auto found = std::ranges::find(moveTypeDataStore, name, &MoveType::name);
	assert(found != moveTypeDataStore.end());
	return *found;
}
void to_json(Json& data, const MoveType* const& moveType){ data = moveType->name; }
void from_json(const Json& data, const MoveType*& moveType){ moveType = &MoveType::byName(data.get<std::string>()); }
