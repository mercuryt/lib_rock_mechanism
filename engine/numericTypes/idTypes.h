#pragma once
#include "strongInteger.h"
#include "dataStructures/smallSet.h"
#include <cstdint>
using AreaIdWidth = uint32_t;
class AreaId : public StrongInteger<AreaId, AreaIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AreaId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AreaId& index) { data = index.get(); }
inline void from_json(const Json& data, AreaId& index) { index = AreaId::create(data.get<AreaIdWidth>()); }

using ItemIdWidth = uint32_t;
class ItemId : public StrongInteger<ItemId, ItemIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ItemId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ItemId& index) { data = index.get(); }
inline void from_json(const Json& data, ItemId& index) { index = ItemId::create(data.get<ItemIdWidth>()); }

using ActorIdWidth = uint32_t;
class ActorId : public StrongInteger<ActorId, ActorIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ActorId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ActorId& index) { data = index.get(); }
inline void from_json(const Json& data, ActorId& index) { index = ActorId::create(data.get<ActorIdWidth>()); }

using VisionCuboidIdWidth = uint32_t;
class VisionCuboidId : public StrongInteger<VisionCuboidId, VisionCuboidIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const VisionCuboidId& index) const { return index.get(); } };
	NLOHMANN_DEFINE_TYPE_INTRUSIVE(VisionCuboidId, data);
};

using FactionIdWidth = uint16_t;
class FactionId : public StrongInteger<FactionId, FactionIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const FactionId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const FactionId& index) { data = index.get(); }
inline void from_json(const Json& data, FactionId& index) { index = FactionId::create(data.get<FactionIdWidth>()); }

using ObjectiveTypeIdWidth = uint8_t;
class ObjectiveTypeId : public StrongInteger<ObjectiveTypeId, ObjectiveTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ObjectiveTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const ObjectiveTypeId& index);
void from_json(const Json& data, ObjectiveTypeId& index);

using PlantSpeciesIdWidth = int16_t;
class PlantSpeciesId : public StrongInteger<PlantSpeciesId, PlantSpeciesIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const PlantSpeciesId& index) const { return index.get(); } };
};
void to_json(Json& data, const PlantSpeciesId& index);
void from_json(const Json& data, PlantSpeciesId& index);

using AnimalSpeciesIdWidth = uint16_t;
class AnimalSpeciesId : public StrongInteger<AnimalSpeciesId, AnimalSpeciesIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AnimalSpeciesId& index) const { return index.get(); } };
};
void to_json(Json& data, const AnimalSpeciesId& index);
void from_json(const Json& data, AnimalSpeciesId& index);

using MaterialTypeIdWidth = uint16_t;
class MaterialTypeId : public StrongInteger<MaterialTypeId, MaterialTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MaterialTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const MaterialTypeId& index);
void from_json(const Json& data, MaterialTypeId& index);

using MaterialCategoryTypeIdWidth = uint16_t;
class MaterialCategoryTypeId : public StrongInteger<MaterialCategoryTypeId, MaterialCategoryTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MaterialCategoryTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const MaterialCategoryTypeId& index);
void from_json(const Json& data, MaterialCategoryTypeId& index);

using SpoilsDataTypeIdWidth = uint16_t;
class SpoilsDataTypeId : public StrongInteger<SpoilsDataTypeId, SpoilsDataTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SpoilsDataTypeId& index) const { return index.get(); } };
};

using ItemTypeIdWidth = uint16_t;
class ItemTypeId : public StrongInteger<ItemTypeId, ItemTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ItemTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const ItemTypeId& index);
void from_json(const Json& data, ItemTypeId& index);

using FluidTypeIdWidth = uint16_t;
class FluidTypeId : public StrongInteger<FluidTypeId, FluidTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const FluidTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const FluidTypeId& index);
void from_json(const Json& data, FluidTypeId& index);

using ShapeIdWidth = uint16_t;
class ShapeId : public StrongInteger<ShapeId, ShapeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ShapeId& index) const { return index.get(); } };
};
void to_json(Json& data, const ShapeId& index);
void from_json(const Json& data, ShapeId& index);

using SkillTypeIdWidth = uint16_t;
class SkillTypeId : public StrongInteger<SkillTypeId, SkillTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SkillTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const SkillTypeId& index);
void from_json(const Json& data, SkillTypeId& index);

using MoveTypeIdWidth = uint16_t;
class MoveTypeId : public StrongInteger<MoveTypeId, MoveTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MoveTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const MoveTypeId& index);
void from_json(const Json& data, MoveTypeId& index);

using BodyTypeIdWidth = uint8_t;
class BodyTypeId : public StrongInteger<BodyTypeId, BodyTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const BodyTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const BodyTypeId& index);
void from_json(const Json& data, BodyTypeId& index);

using BodyPartTypeIdWidth = uint8_t;
class BodyPartTypeId : public StrongInteger<BodyPartTypeId, BodyPartTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const BodyPartTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const BodyPartTypeId& index);
void from_json(const Json& data, BodyPartTypeId& index);

using CraftStepTypeIdWidth = uint16_t;
class CraftStepTypeCategoryId : public StrongInteger<CraftStepTypeCategoryId, CraftStepTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const CraftStepTypeCategoryId& index) const { return index.get(); } };
};
void to_json(Json& data, const CraftStepTypeCategoryId& index);
void from_json(const Json& data, CraftStepTypeCategoryId& index);

using CraftJobTypeIdWidth = uint16_t;
class CraftJobTypeId : public StrongInteger<CraftJobTypeId, CraftJobTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const CraftJobTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const CraftJobTypeId& index);
void from_json(const Json& data, CraftJobTypeId& index);

using AttackTypeIdWidth = uint16_t;
class AttackTypeId : public StrongInteger<AttackTypeId, AttackTypeIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AttackTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const AttackTypeId& index);
void from_json(const Json& data, AttackTypeId& index);

using MaterialTypeConstructionDataIdWidth = uint16_t;
class MaterialTypeConstructionDataId : public StrongInteger<MaterialTypeConstructionDataId, MaterialTypeConstructionDataIdWidth>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MaterialTypeConstructionDataId& index) const { return index.get(); } };
};