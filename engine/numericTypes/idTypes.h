#pragma once
#include "../strongInteger.h"
#include <cstdint>
using AreaIdWidth = int;
class AreaId : public StrongInteger<AreaId, AreaIdWidth, INT32_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AreaId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const AreaId& index) { data = index.get(); }
inline void from_json(const Json& data, AreaId& index) { index = AreaId::create(data.get<AreaIdWidth>()); }

using ItemIdWidth = int;
class ItemId : public StrongInteger<ItemId, ItemIdWidth, INT32_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ItemId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ItemId& index) { data = index.get(); }
inline void from_json(const Json& data, ItemId& index) { index = ItemId::create(data.get<ItemIdWidth>()); }

using ActorIdWidth = int;
class ActorId : public StrongInteger<ActorId, ActorIdWidth, INT32_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ActorId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const ActorId& index) { data = index.get(); }
inline void from_json(const Json& data, ActorId& index) { index = ActorId::create(data.get<ActorIdWidth>()); }

using VisionCuboidIdWidth = int;
class VisionCuboidId : public StrongInteger<VisionCuboidId, VisionCuboidIdWidth, INT32_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const VisionCuboidId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const VisionCuboidId& index) { data = index.get(); }
inline void from_json(const Json& data, VisionCuboidId& index) { index = VisionCuboidId::create(data.get<VisionCuboidIdWidth>()); }

using FactionIdWidth = int16_t;
class FactionId : public StrongInteger<FactionId, FactionIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const FactionId& index) const { return index.get(); } };
};
inline void to_json(Json& data, const FactionId& index) { data = index.get(); }
inline void from_json(const Json& data, FactionId& index) { index = FactionId::create(data.get<FactionIdWidth>()); }

using ObjectiveTypeIdWidth = int;
class ObjectiveTypeId : public StrongInteger<ObjectiveTypeId, ObjectiveTypeIdWidth, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ObjectiveTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const ObjectiveTypeId& index);
void from_json(const Json& data, ObjectiveTypeId& index);

using PlantSpeciesIdWidth = int16_t;
class PlantSpeciesId : public StrongInteger<PlantSpeciesId, PlantSpeciesIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const PlantSpeciesId& index) const { return index.get(); } };
};
void to_json(Json& data, const PlantSpeciesId& index);
void from_json(const Json& data, PlantSpeciesId& index);

using AnimalSpeciesIdWidth = int16_t;
class AnimalSpeciesId : public StrongInteger<AnimalSpeciesId, AnimalSpeciesIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AnimalSpeciesId& index) const { return index.get(); } };
};
void to_json(Json& data, const AnimalSpeciesId& index);
void from_json(const Json& data, AnimalSpeciesId& index);

using MaterialTypeIdWidth = int16_t;
class MaterialTypeId : public StrongInteger<MaterialTypeId, MaterialTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MaterialTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const MaterialTypeId& index);
void from_json(const Json& data, MaterialTypeId& index);
static_assert(HasToJsonMethod<MaterialTypeId>);
static_assert(HasFromJsonMethod<MaterialTypeId>);

using MaterialCategoryTypeIdWidth = int16_t;
class MaterialCategoryTypeId : public StrongInteger<MaterialCategoryTypeId, MaterialCategoryTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MaterialCategoryTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const MaterialCategoryTypeId& index);
void from_json(const Json& data, MaterialCategoryTypeId& index);

using SpoilsDataTypeIdWidth = int16_t;
class SpoilsDataTypeId : public StrongInteger<SpoilsDataTypeId, SpoilsDataTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SpoilsDataTypeId& index) const { return index.get(); } };
};

using ItemTypeIdWidth = int16_t;
class ItemTypeId : public StrongInteger<ItemTypeId, ItemTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ItemTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const ItemTypeId& index);
void from_json(const Json& data, ItemTypeId& index);

using FluidTypeIdWidth = int16_t;
class FluidTypeId : public StrongInteger<FluidTypeId, FluidTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const FluidTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const FluidTypeId& index);
void from_json(const Json& data, FluidTypeId& index);

using ShapeIdWidth = int16_t;
class ShapeId : public StrongInteger<ShapeId, ShapeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const ShapeId& index) const { return index.get(); } };
};
void to_json(Json& data, const ShapeId& index);
void from_json(const Json& data, ShapeId& index);

using SkillTypeIdWidth = int;
class SkillTypeId : public StrongInteger<SkillTypeId, SkillTypeIdWidth, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const SkillTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const SkillTypeId& index);
void from_json(const Json& data, SkillTypeId& index);

using MoveTypeIdWidth = int16_t;
class MoveTypeId : public StrongInteger<MoveTypeId, MoveTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MoveTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const MoveTypeId& index);
void from_json(const Json& data, MoveTypeId& index);

using BodyTypeIdWidth = int;
class BodyTypeId : public StrongInteger<BodyTypeId, BodyTypeIdWidth, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const BodyTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const BodyTypeId& index);
void from_json(const Json& data, BodyTypeId& index);

using BodyPartTypeIdWidth = int;
class BodyPartTypeId : public StrongInteger<BodyPartTypeId, BodyPartTypeIdWidth, INT_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const BodyPartTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const BodyPartTypeId& index);
void from_json(const Json& data, BodyPartTypeId& index);

using CraftStepTypeIdWidth = int16_t;
class CraftStepTypeCategoryId : public StrongInteger<CraftStepTypeCategoryId, CraftStepTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const CraftStepTypeCategoryId& index) const { return index.get(); } };
};
void to_json(Json& data, const CraftStepTypeCategoryId& index);
void from_json(const Json& data, CraftStepTypeCategoryId& index);

using CraftJobTypeIdWidth = int16_t;
class CraftJobTypeId : public StrongInteger<CraftJobTypeId, CraftJobTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const CraftJobTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const CraftJobTypeId& index);
void from_json(const Json& data, CraftJobTypeId& index);

using AttackTypeIdWidth = int16_t;
class AttackTypeId : public StrongInteger<AttackTypeId, AttackTypeIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const AttackTypeId& index) const { return index.get(); } };
};
void to_json(Json& data, const AttackTypeId& index);
void from_json(const Json& data, AttackTypeId& index);

using MaterialTypeConstructionDataIdWidth = int16_t;
class MaterialTypeConstructionDataId : public StrongInteger<MaterialTypeConstructionDataId, MaterialTypeConstructionDataIdWidth, INT16_MAX, 0>
{
public:
	struct Hash { [[nodiscard]] size_t operator()(const MaterialTypeConstructionDataId& index) const { return index.get(); } };
};