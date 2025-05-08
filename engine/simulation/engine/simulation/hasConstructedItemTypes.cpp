#include "hasConstructedItemTypes.h"
#include "../items/constructed.h"
#include "../moveType.h"
#include "../area/area.h"
ItemIndex SimulationHasConstructedItemTypes::createShip(Area& area, const BlockIndex& block, const Facing4& facing, const std::string& name, const FactionId& faction)
{
	static const MoveTypeId moveTypeNone = MoveType::byName("none");
	ConstructedShape constructedShape = ConstructedShape::makeForKeelBlock(area, block, facing);
	ItemTypeParamaters itemTypeParams{
		.name="ship" + std::to_string(++m_nextNameNumber),
		.shape=constructedShape.getOccupiedShape(),
		.moveType=moveTypeNone,
		.motiveForce=constructedShape.getMotiveForce(),
		.volume=constructedShape.getFullDisplacement(),
		// TODO: create block feature type 'cargo hold'.
		.internalVolume=FullDisplacement::create(0),
		.value=constructedShape.getValue(),
		.installable=false,
		.generic=false,
		.canHoldFluids=false,
	};
	itemTypeParams.constructedShape = std::move(constructedShape);
	ItemTypeId itemType = ItemType::create(itemTypeParams);
	m_data.insert(itemType, itemTypeParams);
	ItemParamaters itemParams{
		.itemType=itemType,
		.materialType=MaterialTypeId::null(),
		.faction=faction,
		.location=constructedShape.getLocation(),
		.percentWear=Percent::create(0),
		.facing=facing,
		.name=name
	};
	return area.getItems().create(itemParams);
}