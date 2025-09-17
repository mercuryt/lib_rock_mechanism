#include "strongVector.hpp"
#include "rtreeBoolean.h"
#include "rtreeBooleanLowZOnly.h"
#include "../numericTypes/idTypes.h"
#include "../numericTypes/index.h"
#include "../reference.h"
#include "../geometry/cuboidSet.h"
#include "../definitions/moveType.h"
#include "../definitions/shape.h"
#include "../body.h"
#include "../craft.h"
#include "../items/constructed.h"

template class StrongVector<BodyTypeId, AnimalSpeciesId>;
template class StrongVector<CombatScore, AttackTypeId>;
template class StrongVector<CraftStepTypeCategoryId, ItemTypeId>;
template class StrongVector<Density, FluidTypeId>;
template class StrongVector<Density, MaterialTypeId>;
template class StrongVector<Distance, AnimalSpeciesId>;
template class StrongVector<Distance, FluidTypeId>;
template class StrongVector<Distance, PlantSpeciesId>;
template class StrongVector<DistanceFractional, AttackTypeId>;
template class StrongVector<FluidTypeId, AnimalSpeciesId>;
template class StrongVector<FluidTypeId, ItemTypeId>;
template class StrongVector<FluidTypeId, MaterialTypeId>;
template class StrongVector<FluidTypeId, PlantSpeciesId>;
template class StrongVector<Force, AttackTypeId>;
template class StrongVector<Force, ItemTypeId>;
template class StrongVector<FullDisplacement, BodyPartTypeId>;
template class StrongVector<FullDisplacement, ItemTypeId>;
template class StrongVector<FullDisplacement, PlantSpeciesId>;
template class StrongVector<ItemTypeId, AttackTypeId>;
template class StrongVector<ItemTypeId, CraftJobTypeId>;
template class StrongVector<ItemTypeId, PlantSpeciesId>;
template class StrongVector<ItemTypeId, SpoilsDataTypeId>;
template class StrongVector<Mass, PlantSpeciesId>;
template class StrongVector<MaterialCategoryTypeId, CraftJobTypeId>;
template class StrongVector<MaterialCategoryTypeId, MaterialTypeId>;
template class StrongVector<MaterialTypeId, AnimalSpeciesId>;
template class StrongVector<MaterialTypeId, FluidTypeId>;
template class StrongVector<MaterialTypeId, PlantSpeciesId>;
template class StrongVector<MaterialTypeId, SpoilsDataTypeId>;
template class StrongVector<MoveTypeId, AnimalSpeciesId>;
template class StrongVector<MoveTypeId, ItemTypeId>;
template class StrongVector<MoveTypeParamaters, MoveTypeId>;
template class StrongVector<OffsetCuboidSet, ItemTypeId>;
template class StrongVector<Percent, ItemTypeId>;
template class StrongVector<Percent, SpoilsDataTypeId>;
template class StrongVector<Quantity, CraftJobTypeId>;
template class StrongVector<Quantity, PlantSpeciesId>;
template class StrongVector<Quantity, SpoilsDataTypeId>;
template class StrongVector<ShapeId, ItemTypeId>;
template class StrongVector<SkillExperiencePoints, SkillTypeId>;
template class StrongVector<SkillTypeId, AttackTypeId>;
template class StrongVector<SkillTypeId, ItemTypeId>;
template class StrongVector<SkillTypeId, MaterialTypeId>;
template class StrongVector<SmallMap<FluidTypeId, CollisionVolume>, MoveTypeId>;
template class StrongVector<SmallSet<FluidTypeId>, MoveTypeId>;
template class StrongVector<MapWithOffsetCuboidKeys<CollisionVolume>, ShapeId>;
template class StrongVector<MapWithCuboidKeys<CollisionVolume>, ShapeId>;
template class StrongVector<Step, AnimalSpeciesId>;
template class StrongVector<Step, AttackTypeId>;
template class StrongVector<Step, FluidTypeId>;
template class StrongVector<Step, ItemTypeId>;
template class StrongVector<Step, MaterialTypeId>;
template class StrongVector<Step, PlantSpeciesId>;
template class StrongVector<Temperature, AnimalSpeciesId>;
template class StrongVector<Temperature, MaterialTypeId>;
template class StrongVector<Temperature, PlantSpeciesId>;
template class StrongVector<TemperatureDelta, MaterialTypeId>;
template class StrongVector<WoundType, AttackTypeId>;
template class StrongVector<float, FluidTypeId>;
template class StrongVector<float, SkillTypeId>;
template class StrongVector<std::array<AttributeLevel, 3>, AnimalSpeciesId>;
template class StrongVector<std::array<Mass, 3>, AnimalSpeciesId>;
template class StrongVector<std::array<Step, 2>, AnimalSpeciesId>;
template class StrongVector<std::array<int32_t, 3>, ItemTypeId>;
template class StrongVector<std::array<uint32_t, 3>, AnimalSpeciesId>;
template class StrongVector<std::string, AnimalSpeciesId>;
template class StrongVector<std::string, AttackTypeId>;
template class StrongVector<std::string, BodyPartTypeId>;
template class StrongVector<std::string, BodyTypeId>;
template class StrongVector<std::string, CraftJobTypeId>;
template class StrongVector<std::string, CraftStepTypeCategoryId>;
template class StrongVector<std::string, FluidTypeId>;
template class StrongVector<std::string, ItemTypeId>;
template class StrongVector<std::string, MaterialCategoryTypeId>;
template class StrongVector<std::string, MaterialTypeId>;
template class StrongVector<std::string, MoveTypeId>;
template class StrongVector<std::string, PlantSpeciesId>;
template class StrongVector<std::string, ShapeId>;
template class StrongVector<std::string, SkillTypeId>;
template class StrongVector<std::unique_ptr<ConstructedShape>, ItemTypeId>;
template class StrongVector<std::vector<AttackTypeId>, ItemTypeId>;
template class StrongVector<std::vector<BodyPartTypeId>, BodyTypeId>;
template class StrongVector<std::vector<BodyPartTypeId>, ItemTypeId>;
template class StrongVector<std::vector<CraftStepType>, CraftJobTypeId>;
template class StrongVector<std::vector<MaterialCategoryTypeId>, ItemTypeId>;
template class StrongVector<std::vector<ShapeId>, AnimalSpeciesId>;
template class StrongVector<std::vector<ShapeId>, PlantSpeciesId>;
template class StrongVector<std::vector<SpoilsDataTypeId>, MaterialTypeId>;
template class StrongVector<std::vector<std::pair<AttackTypeId, MaterialTypeId>>, BodyPartTypeId>;
template class StrongVector<std::vector<std::pair<ItemQuery, Quantity>>, MaterialTypeId>;
template class StrongVector<std::vector<std::tuple<ItemTypeId, MaterialTypeId, Quantity>>, MaterialTypeId>;
template class StrongVector<uint16_t, PlantSpeciesId>;
template class StrongVector<uint32_t, AnimalSpeciesId>;
template class StrongVector<uint32_t, AttackTypeId>;
template class StrongVector<uint32_t, FluidTypeId>;
template class StrongVector<uint32_t, ItemTypeId>;
template class StrongVector<uint32_t, MaterialTypeId>;
template class StrongVector<uint32_t, ShapeId>;
template class StrongVector<uint8_t, MoveTypeId>;
template class StrongVector<uint8_t, PlantSpeciesId>;
template class StrongVector<RTreeBoolean::Node, RTreeBoolean::Index>;
template class StrongVector<RTreeBooleanLowZOnly::Node, RTreeBooleanLowZOnly::Index>;

/*
TODO: These are commented out because std::array reports a false positive to std::equality_comparable.
template class StrongVector<std::array<SmallSet<Offset3D>,4>, ShapeId>;
template class StrongVector<std::array<SmallSet<OffsetAndVolume>,4>, ShapeId>;
*/

template class StrongBitSet<PlantSpeciesId>;
template class StrongBitSet<AnimalSpeciesId>;
template class StrongBitSet<AttackTypeId>;
template class StrongBitSet<BodyPartTypeId>;
template class StrongBitSet<ItemTypeId>;
template class StrongBitSet<MaterialTypeId>;
template class StrongBitSet<MoveTypeId>;
template class StrongBitSet<ShapeId>;
template class StrongBitSet<ActorIndex>;
template class StrongBitSet<ItemIndex>;