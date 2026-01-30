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
#include "../fire.h"
#include "../objective.h"
#include "../projects/installItem.h"
#include "../squad.h"
#include "rtreeData.hpp"

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

//template class std::vector<std::pair<ActorOrItemIndex, DeckRotationDataSingle>>;
template class std::vector<std::pair<ActorReference, ProjectWorker>>;
template class std::vector<std::pair<ActorReference, SmallMap<ProjectRequirementCounts*, ItemReference>>>;
template class std::vector<std::pair<AreaId, Area*>>;
template class std::vector<std::pair<AreaId, std::vector<DramaArc*>>>;
template class std::vector<std::pair<CanReserve*, Quantity>>;
template class std::vector<std::pair<Point3D, Point3D>>;
//template class std::vector<std::pair<CanReserve*, std::unique_ptr<DishonorCallback>>>;
template class std::vector<std::pair<CraftStepTypeCategoryId, SmallSet<Point3D>>>;
template class std::vector<std::pair<CraftStepTypeCategoryId, std::vector<CraftJob*>>>;
template class std::vector<std::pair<DeckId, CuboidSetAndActorOrItemIndex>>;
template class std::vector<std::pair<FactionId, AreaHasSpaceDesignationsForFaction>>;
template class std::vector<std::pair<FactionId, AreaHasStocksForFaction>>;
template class std::vector<std::pair<FactionId, HasFarmFieldsForFaction>>;
//template class std::vector<std::pair<FactionId, HasScheduledEvent<ReMarkItemForStockPilingEvent>>>;
template class std::vector<std::pair<FactionId, PointIsPartOfStockPile>>;
template class std::vector<std::pair<FactionId, Quantity>>;
template class std::vector<std::pair<FactionId, RTreeData<RTreeDataWrapper<Project*, nullptr>>>>;
template class std::vector<std::pair<FactionId, SimulationHasUniformsForFaction>>;
template class std::vector<std::pair<FactionId, SmallMap<SpaceDesignation, Quantity>>>;
template class std::vector<std::pair<FactionId, StrongVector<Squad, SquadIndex>>>;
//template class std::vector<std::pair<FactionId, HasOnSight>>;

//template struct SmallMap<FactionId, RTreeDataIndex<SmallSet<Project*>, int, noMerge>>;
template struct SmallMap<ActorOrItemIndex, DeckRotationDataSingle>;
template struct SmallMap<ActorReference, ProjectWorker>;
template struct SmallMap<ActorReference, SmallMap<ProjectRequirementCounts*, ItemReference>>;
template struct SmallMap<AreaId, Area*>;
template struct SmallMap<AreaId, std::vector<DramaArc*>>;
template struct SmallMap<CanReserve*, Quantity>;
template struct SmallMap<CanReserve*, std::unique_ptr<DishonorCallback>>;
template struct SmallMap<CraftStepTypeCategoryId, SmallSet<Point3D>>;
template struct SmallMap<CraftStepTypeCategoryId, std::vector<CraftJob*>>;
template struct SmallMap<DeckId, CuboidSetAndActorOrItemIndex>;
template struct SmallMap<FactionId, AreaHasSpaceDesignationsForFaction>;
template struct SmallMap<FactionId, AreaHasStocksForFaction>;
template struct SmallMap<FactionId, HasFarmFieldsForFaction>;
template struct SmallMap<FactionId, HasScheduledEvent<ReMarkItemForStockPilingEvent>>;
template struct SmallMap<FactionId, HasOnSight>;
template struct SmallMap<FactionId, PointIsPartOfStockPile>;
template struct SmallMap<FactionId, Quantity>;
template struct SmallMap<FactionId, RTreeData<RTreeDataWrapper<Project*, nullptr>>>;
template struct SmallMap<FactionId, SimulationHasUniformsForFaction>;
template struct SmallMap<FactionId, SmallMap<SpaceDesignation, Quantity>>;
template struct SmallMap<FactionId, StrongVector<Squad, SquadIndex>>;
template struct SmallMap<FluidGroup*, SmallSet<Point3D>>;
template struct SmallMap<FluidTypeId, CollisionVolume>;
template struct SmallMap<FluidTypeId, FluidGroup*>;
template struct SmallMap<FluidTypeId, FluidRequirementData>;
template struct SmallMap<FluidTypeId, SmallSet<Point3D>>;
template struct SmallMap<ItemReference, Quantity>;
template struct SmallMap<ItemReference, SmallSet<StockPileProject*>>;
template struct SmallMap<ItemReference, std::pair<ProjectRequirementCounts*, Quantity>>;
template struct SmallMap<ItemTypeId, ItemTypeParamaters>;
template struct SmallMap<ItemTypeId, SmallMap<MaterialTypeId, SmallSet<ItemIndex>>>;
template struct SmallMap<ItemTypeId, SmallSet<ItemReference>>;
template struct SmallMap<ItemTypeId, SmallSet<StockPile*>>;
template struct SmallMap<Offset3D, MaterialTypeId>;
template struct SmallMap<Offset3D, PointFeature>;
template struct SmallMap<Point3D, Point3D>;
template struct SmallMap<Point3D, Reservable*>;
template struct SmallMap<Point3D, SmallMap<FactionId, FarmField*>>;
template struct SmallMap<Point3D, SmallMap<FactionId, PointIsPartOfStockPile>>;
template struct SmallMap<Point3D, SmallMap<FluidTypeId, CollisionVolume>>;
template struct SmallMap<Point3D, TemperatureDelta>;
template struct SmallMap<Point3D, TemperatureSource>;
template struct SmallMap<Point3D, std::unique_ptr<DishonorCallback>>;
template struct SmallMap<Point3D, std::vector<CraftStepTypeCategoryId>>;
template struct SmallMap<Project*, DeckRotationDataSingle>;
template struct SmallMap<RTreeNodeIndex, SmallSet<int>>;
template struct SmallMap<SkillTypeId, Skill>;
template struct SmallMap<SkillTypeId, std::vector<CraftJob*>>;
template struct SmallMap<StockPile*, SmallSet<ItemReference>>;
template struct SmallMap<StockPile*, SmallSet<Point3D>>;
template struct SmallMap<std::string, Uniform>;
template struct SmallMap<int, int>;

template struct SmallMapStable<AreaId, Area>;
template struct SmallMapStable<FactionId, AreaHasStockPilesForFaction>;
template struct SmallMapStable<FactionId, HasConstructionDesignationsForFaction>;
template struct SmallMapStable<FactionId, HasDigDesignationsForFaction>;
template struct SmallMapStable<FactionId, HasWoodCuttingDesignationsForFaction>;
template struct SmallMapStable<MaterialTypeId, Fire*>;
template struct SmallMapStable<MaterialTypeId, Fire>;
template struct SmallMapStable<NeedType, SupressedNeed>;
template struct SmallMapStable<Point3D, DigProject>;
template struct SmallMapStable<Point3D, InstallItemProject>;
template struct SmallMapStable<Point3D, WoodCuttingProject>;