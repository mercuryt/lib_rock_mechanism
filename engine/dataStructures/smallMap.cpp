#include "smallMap.hpp"
#include "smallSet.h"
#include "rtreeBoolean.h"
#include "../numericTypes/types.h"
#include "../numericTypes/idTypes.h"
#include "../numericTypes/index.h"
#include "../geometry/point3D.h"
#include "../geometry/cuboid.h"
#include "../reference.h"
#include "../designations.h"
#include "../deck.h"
#include "../project.h"
#include "../stocks.h"
#include "../farmFields.h"
#include "../temperature.h"
#include "../area/area.h"
#include "../area/hasSpaceDesignations.h"
#include "../area/stockpile.h"
#include "../area/hasConstructionDesignations.h"
#include "../area/hasDigDesignations.h"
#include "../area/woodcutting.h"
#include "../actors/uniform.h"
#include "../actors/skill.h"
#include "../items/items.h"
#include "../path/longRange.h"
#include "../fire.h"
#include "../objective.h"
#include "../projects/installItem.h"

class Area;
class CraftJob;
class FluidGroup;
class Reservable;
class DramaArc;
class Project;
class StockPile;
class StockPileProject;
class ProjectRequirementCounts;
class FarmField;
class CanReserve;

template class SmallMap<ActorOrItemIndex, DeckRotationDataSingle>;
template class SmallMap<ActorReference, ProjectWorker>;
template class SmallMap<ActorReference, SmallMap<ProjectRequirementCounts*, ItemReference>>;
template class SmallMap<AreaId, Area*>;
template class SmallMap<AreaId, std::vector<DramaArc*>>;
template class SmallMap<CanReserve*, Quantity>;
template class SmallMap<CanReserve*, std::unique_ptr<DishonorCallback>>;
template class SmallMap<CraftStepTypeCategoryId, SmallSet<Point3D>>;
template class SmallMap<CraftStepTypeCategoryId, std::vector<CraftJob*>>;
template class SmallMap<DeckId, CuboidSetAndActorOrItemIndex>;
template class SmallMap<FactionId, AreaHasSpaceDesignationsForFaction>;
template class SmallMap<FactionId, AreaHasStocksForFaction>;
template class SmallMap<FactionId, HasFarmFieldsForFaction>;
template class SmallMap<FactionId, HasScheduledEvent<ReMarkItemForStockPilingEvent>>;
template class SmallMap<FactionId, PointIsPartOfStockPile>;
template class SmallMap<FactionId, Quantity>;
//template class SmallMap<FactionId, RTreeDataIndex<SmallSet<Project*>, uint16_t, noMerge>>;
template class SmallMap<FactionId, SimulationHasUniformsForFaction>;
template class SmallMap<FactionId, SmallMap<SpaceDesignation, Quantity>>;
template class SmallMap<FluidGroup*, SmallSet<Point3D>>;
template class SmallMap<FluidTypeId, CollisionVolume>;
template class SmallMap<FluidTypeId, FluidGroup*>;
template class SmallMap<FluidTypeId, FluidRequirementData>;
template class SmallMap<FluidTypeId, SmallSet<Point3D>>;
template class SmallMap<ItemReference, Quantity>;
template class SmallMap<ItemReference, SmallSet<StockPileProject*>>;
template class SmallMap<ItemReference, std::pair<ProjectRequirementCounts*, Quantity>>;
template class SmallMap<ItemTypeId, ItemTypeParamaters>;
template class SmallMap<ItemTypeId, SmallMap<MaterialTypeId, SmallSet<ItemIndex>>>;
template class SmallMap<ItemTypeId, SmallSet<ItemReference>>;
template class SmallMap<ItemTypeId, SmallSet<StockPile*>>;
template class SmallMap<LongRangePathNodeIndex, LongRangePathNodeIndex>;
template class SmallMap<MoveTypeId, LongRangePathFinderForMoveType>;
template class SmallMap<Offset3D, MaterialTypeId>;
template class SmallMap<Offset3D, PointFeatureSet>;
template class SmallMap<Point3D, Reservable*>;
template class SmallMap<Point3D, SmallMap<FactionId, FarmField*>>;
template class SmallMap<Point3D, SmallMap<FactionId, PointIsPartOfStockPile>>;
template class SmallMap<Point3D, SmallMap<FluidTypeId, CollisionVolume>>;
template class SmallMap<Point3D, TemperatureDelta>;
template class SmallMap<Point3D, TemperatureSource>;
template class SmallMap<Point3D, std::unique_ptr<DishonorCallback>>;
template class SmallMap<Point3D, std::vector<CraftStepTypeCategoryId>>;
template class SmallMap<Project*, DeckRotationDataSingle>;
template class SmallMap<SkillTypeId, Skill>;
template class SmallMap<SkillTypeId, std::vector<CraftJob*>>;
template class SmallMap<StockPile*, SmallSet<ItemReference>>;
template class SmallMap<StockPile*, SmallSet<Point3D>>;
template class SmallMap<VisionCuboidId, Cuboid>;
template class SmallMap<std::string, Uniform>;
template class SmallMap<uint, uint>;
template class SmallMap<RTreeBoolean::Index, SmallSet<uint>>;

template class SmallMapStable<AreaId, Area>;
template class SmallMapStable<FactionId, AreaHasStockPilesForFaction>;
template class SmallMapStable<FactionId, HasConstructionDesignationsForFaction>;
template class SmallMapStable<FactionId, HasDigDesignationsForFaction>;
template class SmallMapStable<FactionId, HasWoodCuttingDesignationsForFaction>;
template class SmallMapStable<MaterialTypeId, Fire*>;
template class SmallMapStable<MaterialTypeId, Fire>;
template class SmallMapStable<NeedType, SupressedNeed>;
template class SmallMapStable<Point3D, DigProject>;
template class SmallMapStable<Point3D, InstallItemProject>;
template class SmallMapStable<Point3D, WoodCuttingProject>;