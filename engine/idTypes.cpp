// Store type ids by name so adding new ones doesn't break old saves.
#include "idTypes.h"
#include "objective.h"
void to_json(Json& data, const ObjectiveTypeId& id) { data = ObjectiveType::getById(id).name(); }
void from_json(const Json& data, ObjectiveTypeId& id) { id = ObjectiveType::getIdByName(data.get<std::string>()); }
#include "plantSpecies.h"
void to_json(Json& data, const PlantSpeciesId& id) { data = PlantSpecies::getName(id); }
void from_json(const Json& data, PlantSpeciesId& id) { id = PlantSpecies::byName(data.get<std::string>()); }
#include "animalSpecies.h"
void to_json(Json& data, const AnimalSpeciesId& id) { data = AnimalSpecies::getName(id); }
void from_json(const Json& data, AnimalSpeciesId& id) { id = AnimalSpecies::byName(data.get<std::string>()); }
#include "materialType.h"
void to_json(Json& data, const MaterialTypeId& id) { data = MaterialType::getName(id); }
void from_json(const Json& data, MaterialTypeId& id) { id = MaterialType::byName(data.get<std::string>()); }
void to_json(Json& data, const MaterialCategoryTypeId& id) { data = MaterialTypeCategory::getName(id); }
void from_json(const Json& data, MaterialCategoryTypeId& id) { id = MaterialTypeCategory::byName(data.get<std::string>()); }
#include "itemType.h"
void to_json(Json& data, const ItemTypeId& id) { data = ItemType::getName(id); }
void from_json(const Json& data, ItemTypeId& id) { id = ItemType::byName(data.get<std::string>()); }
#include "fluidType.h"
void to_json(Json& data, const FluidTypeId& id) { data = FluidType::maybeGetName(id); }
void from_json(const Json& data, FluidTypeId& id) { id = FluidType::byName(data.get<std::string>()); }
#include "shape.h"
void to_json(Json& data, const ShapeId& id) { data = id.exists() ? Shape::getName(id) : "Null"; }
void from_json(const Json& data, ShapeId& id) { const auto& string = data.get<std::string>(); id = (string == "Null") ? ShapeId::null() : Shape::byName(string); }
#include "skill.h"
void to_json(Json& data, const SkillTypeId& id) { data = SkillType::getName(id); }
void from_json(const Json& data, SkillTypeId& id) { id = SkillType::byName(data.get<std::string>()); }
#include "moveType.h"
void to_json(Json& data, const MoveTypeId& id) { data = MoveType::getName(id); }
void from_json(const Json& data, MoveTypeId& id) { id = MoveType::byName(data.get<std::string>()); }
#include "bodyType.h"
void to_json(Json& data, const BodyTypeId& id) { data = BodyType::getName(id); }
void from_json(const Json& data, BodyTypeId& id) { id = BodyType::byName(data.get<std::string>()); }
void to_json(Json& data, const BodyPartTypeId& id) { data = BodyPartType::getName(id); }
void from_json(const Json& data, BodyPartTypeId& id) { id = BodyPartType::byName(data.get<std::string>()); }
#include "craft.h"
void to_json(Json& data, const CraftStepTypeCategoryId& id) { data = CraftStepTypeCategory::getName(id); }
void from_json(const Json& data, CraftStepTypeCategoryId& id) { id = CraftStepTypeCategory::byName(data.get<std::string>()); }
void to_json(Json& data, const CraftJobTypeId& id) { data = CraftJobType::getName(id); }
void from_json(const Json& data, CraftJobTypeId& id) { id = CraftJobType::byName(data.get<std::string>()); }
#include "attackType.h"
void to_json(Json& data, const AttackTypeId& id) { data = AttackType::getName(id); }
void from_json(const Json& data, AttackTypeId& id) { id = AttackType::byName(data.get<std::string>()); }
