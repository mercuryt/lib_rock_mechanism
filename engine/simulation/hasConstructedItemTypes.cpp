#include "hasConstructedItemTypes.h"
#include "../items/constructed.h"
#include "../items/items.h"
#include "../definitions/moveType.h"
#include "../area/area.h"
void SimulationHasConstructedItemTypes::load()
{
	for(const auto& [itemTypeId, itemTypeParamaters] : m_data)
		ItemType::create(itemTypeId, itemTypeParamaters);
}
ItemIndex SimulationHasConstructedItemTypes::createShip(Area& area, const Point3D& point, const Facing4& facing, const std::string& name, const FactionId& faction)
{
	static const MoveTypeId moveTypeNone = MoveType::byName("none");
	auto [constructedShape, location] = ConstructedShape::makeForKeelPoint(area, point, facing);
	ItemTypeParamaters itemTypeParams{
		.name="ship" + std::to_string(++m_nextNameNumber),
		.shape=constructedShape.getShape(),
		.moveType=moveTypeNone,
		.motiveForce=constructedShape.getMotiveForce(),
		.volume=constructedShape.getFullDisplacement(),
		// TODO: create point feature type 'cargo hold'.
		.internalVolume=FullDisplacement::create(0),
		.value=constructedShape.getValue(),
		.decks=constructedShape.getDecks(),
		.installable=false,
		.generic=false,
		.canHoldFluids=false,
	};
	itemTypeParams.constructedShape = std::make_unique<ConstructedShape>(std::move(constructedShape));
	ItemTypeId itemType = ItemType::create(itemTypeParams);
	m_data.insert(itemType, std::move(itemTypeParams));
	ItemParamaters itemParams{
		.itemType=itemType,
		.materialType=MaterialTypeId::null(),
		.faction=faction,
		.location=location,
		.percentWear=Percent::create(0),
		.facing=facing,
		.name=name
	};
	return area.getItems().create(itemParams);
}