#include "haul.h"
#include "actor.h"
#include "config.h"
#include "hasShape.h"
#include "item.h"
#include "area.h"
#include "materialType.h"
#include "project.h"
#include "reservable.h"
#include "simulation/hasItems.h"
#include "simulation/hasActors.h"
#include "types.h"
#include "simulation.h"

#include <memory>
#include <sys/types.h>
#include <unordered_set>
#include <algorithm>

HaulStrategy haulStrategyFromName(std::string name)
{
	if(name == "Individual")
		return HaulStrategy::Individual;
	if(name == "Team")
		return HaulStrategy::Team;
	if(name == "IndividualCargoIsCart")
		return HaulStrategy::IndividualCargoIsCart;
	if(name == "Cart")
		return HaulStrategy::Cart;
	if(name == "TeamCart")
		return HaulStrategy::TeamCart;
	if(name == "Panniers")
		return HaulStrategy::Panniers;
	if(name == "AnimalCart")
		return HaulStrategy::AnimalCart;
	assert(name == "StrongSentient");
	return HaulStrategy::StrongSentient;
}
std::string haulStrategyToName(HaulStrategy strategy)
{
	if(strategy == HaulStrategy::Individual)
		return "Individual";
	if(strategy == HaulStrategy::Team)
		return "Team";
	if(strategy == HaulStrategy::IndividualCargoIsCart)
		return "IndividualCargoIsCart";
	if(strategy == HaulStrategy::Cart)
		return "Cart";
	if(strategy == HaulStrategy::TeamCart)
		return "TeamCart";
	if(strategy == HaulStrategy::Panniers)
		return "Panniers";
	if(strategy == HaulStrategy::AnimalCart)
		return "AnimalCart";
	assert(strategy == HaulStrategy::StrongSentient);
	return "StrongSentient";
}
void HaulSubprojectParamaters::reset()
{
	toHaul = nullptr;
	fluidType = nullptr;
	quantity = 0;
	strategy = HaulStrategy::None;
	haulTool = nullptr;
	beastOfBurden = nullptr;
	projectRequirementCounts = nullptr;	
}
// Check that the haul strategy selected in read step is still valid in write step.
bool HaulSubprojectParamaters::validate() const
{
	assert(toHaul != nullptr);
	assert(quantity != 0);
	assert(strategy != HaulStrategy::None);
	assert(projectRequirementCounts != nullptr);
	Faction& faction = *(*workers.begin())->getFaction();
	for(Actor* worker : workers)
		if(worker->m_reservable.isFullyReserved(&faction))
			return false;
	if(haulTool != nullptr && haulTool->m_reservable.isFullyReserved(&faction))
		return false;
	if(beastOfBurden != nullptr && beastOfBurden->m_reservable.isFullyReserved(&faction))
		return false;
	return true;
}
HaulSubprojectDishonorCallback::HaulSubprojectDishonorCallback(const Json data, DeserializationMemo& deserializationMemo) : 
	m_haulSubproject(*deserializationMemo.m_haulSubprojects.at(data["haulSubproject"])) { }
CanPickup::CanPickup(const Json& data, Actor& a, Simulation& s) : m_actor(a)
{
	if(data.contains("carryingItem"))
		m_carrying = &s.m_hasItems->getById(data["carryingItem"].get<ItemId>());
	else if(data.contains("carryingActor"))
		m_carrying = &s.m_hasActors->getById(data["carryingActor"].get<ActorId>());
}
Json CanPickup::toJson() const 
{
	Json data;
	if(m_carrying != nullptr)
	{
		if(m_carrying->isItem())
			data["carryingItem"] = static_cast<Item&>(*m_carrying).m_id;
		else
			data["carryingActor"] = static_cast<Actor&>(*m_carrying).m_id;
	}
	return data;
}
void CanPickup::pickUp(HasShape& hasShape, Quantity quantity)
{
	if(hasShape.isItem())
		pickUp(static_cast<Item&>(hasShape), quantity);
	else
	{
		assert(hasShape.isActor());
		pickUp(static_cast<Actor&>(hasShape), quantity);
	}
}
void CanPickup::pickUp(Item& item, Quantity quantity)
{
	assert(quantity <= item.getQuantity());
	assert(quantity == 1 || item.m_itemType.generic);
	item.m_reservable.maybeClearReservationFor(m_actor.m_canReserve);
	if(quantity == item.getQuantity())
	{
		m_carrying = &item;
		if(item.m_location)
			item.exit();
	}
	else
	{
		item.removeQuantity(quantity);
		Item& newItem = m_actor.getSimulation().m_hasItems->createItem({
			.itemType=item.m_itemType, 
			.materialType=item.m_materialType, 
			.quantity=quantity
		});
		m_carrying = &newItem;
	}
	m_actor.m_canMove.updateIndividualSpeed();
}
void CanPickup::pickUp(Actor& actor, [[maybe_unused]] Quantity quantity)
{
	assert(quantity == 1);
	assert(!actor.m_mustSleep.isAwake() || actor.m_body.isInjured());
	assert(m_carrying = nullptr);
	actor.m_reservable.maybeClearReservationFor(m_actor.m_canReserve);
	if(actor.m_location)
		actor.exit();
	m_carrying = &actor;
	m_actor.m_canMove.updateIndividualSpeed();
}
HasShape& CanPickup::putDown(BlockIndex location, Quantity quantity)
{
	assert(m_carrying != nullptr);
	auto& blocks = m_actor.m_area->getBlocks();
	if(quantity == 0)
		quantity = m_carrying->isItem() ? getItem().getQuantity() : 1u;
	if(m_carrying->isItem())
		assert(quantity <= getItem().getQuantity());
	HasShape* output = nullptr;
	if(m_carrying->isGeneric())
	{
		Item& item = getItem();
		if(item.getQuantity() == quantity)
		{
			m_carrying = nullptr;
			if(blocks.item_getCount(location, item.m_itemType, item.m_materialType) == 0)
			{
				item.setLocation(location);
				output = &item;
			}
			else
				output = &blocks.item_addGeneric(location, item.m_itemType, item.m_materialType, quantity);
		}
		else
		{
			item.removeQuantity(quantity);
			output = &blocks.item_addGeneric(location, item.m_itemType, item.m_materialType, quantity);
		}
	}
	else
	{
		m_carrying->setLocation(location, m_actor.m_area);
		output = m_carrying;
		m_carrying = nullptr;
	}
	m_actor.m_canMove.updateIndividualSpeed();
	return *output;
}
HasShape* CanPickup::putDownIfAny(BlockIndex location)
{
	if(m_carrying != nullptr)
		return &putDown(location);
	return nullptr;
}
void CanPickup::removeFluidVolume(CollisionVolume volume)
{
	assert(m_carrying != nullptr);
	assert(m_carrying->isItem());
	Item& item = getItem();
	assert(item.m_hasCargo.containsAnyFluid());
	assert(item.m_hasCargo.getFluidVolume() >= volume);
	item.m_hasCargo.removeFluidVolume(volume);
}
void CanPickup::add(const ItemType& itemType, const MaterialType& materialType, Quantity quantity)
{
	if(m_carrying == nullptr)
		m_carrying = &m_actor.getSimulation().m_hasItems->createItem({
			.itemType=itemType, 
			.materialType=materialType, 
			.quantity=quantity
		});
	else
	{
		assert(m_carrying->isItem());	
		Item& item = static_cast<Item&>(*m_carrying);
		assert(item.m_itemType.generic && item.m_itemType == itemType && item.m_materialType == materialType);
		item.addQuantity(quantity);
	}
}
void CanPickup::remove([[maybe_unused]] Item& item)
{
	assert(m_carrying == &item);
	m_carrying = nullptr;
}
Item& CanPickup::getItem()
{
	assert(m_carrying != nullptr);
	return static_cast<Item&>(*m_carrying);
}
Actor& CanPickup::getActor()
{
	assert(m_carrying != nullptr);
	return static_cast<Actor&>(*m_carrying);
}
Quantity CanPickup::canPickupQuantityOf(const HasShape& hasShape) const
{
	return maximumNumberWhichCanBeCarriedWithMinimumSpeed(hasShape, 1);
}
Quantity CanPickup::canPickupQuantityOf(const ItemType& itemType, const MaterialType& materialType, Quantity max) const
{
	Mass unitMass = itemType.volume * materialType.density;
	Quantity output = 1;
	while(output < max)
	{
		if(!m_actor.m_canMove.getIndividualMoveSpeedWithAddedMass(unitMass * output))
			return output - 1;
		++output;
	}
	assert(output == max);
	return output;
}
bool CanPickup::canPickupAny(const HasShape& hasShape) const
{
	return m_actor.m_canMove.getIndividualMoveSpeedWithAddedMass(hasShape.singleUnitMass()) > 0;
}
bool CanPickup::canPickupAnyUnencombered(const HasShape& hasShape) const
{
	return m_actor.m_canMove.getIndividualMoveSpeedWithAddedMass(hasShape.singleUnitMass()) == m_actor.m_canMove.getMoveSpeed();
}
bool CanPickup::isCarryingGeneric(const ItemType& itemType, const MaterialType& materialType, Quantity quantity) const
{
	if(m_carrying == nullptr || !m_carrying->isItem())
		return false;
	Item& item = const_cast<CanPickup&>(*this).getItem();
	if(item.m_itemType != itemType)
		return false;
	if(item.m_materialType != materialType)
		return false;
	return item.getQuantity() >= quantity;
}
bool CanPickup::isCarryingFluidType(const FluidType& fluidType) const 
{
	if(m_carrying == nullptr)
		return false;
	Item& item = static_cast<Item&>(*m_carrying);
	if(!item.m_hasCargo.containsAnyFluid())
		return false;
       	return item.m_hasCargo.getFluidType() == fluidType; 
}
const CollisionVolume& CanPickup::getFluidVolume() const 
{ 
	assert(m_carrying != nullptr);
	Item& item = static_cast<Item&>(*m_carrying);
	return item.m_hasCargo.getFluidVolume(); 
}
const FluidType& CanPickup::getFluidType() const 
{ 
	assert(m_carrying != nullptr);
	Item& item = static_cast<Item&>(*m_carrying);
	assert(item.m_hasCargo.containsAnyFluid());
	return item.m_hasCargo.getFluidType(); 
}
bool CanPickup::isCarryingEmptyContainerWhichCanHoldFluid() const 
{ 
	if(m_carrying == nullptr)
		return false;
	assert(m_carrying->isItem());
	Item& item = static_cast<Item&>(*m_carrying);
	return item.m_itemType.canHoldFluids && item.m_hasCargo.empty(); 
}
Mass CanPickup::getMass() const
{
	if(m_carrying == nullptr)
	{
		constexpr static Mass zero = 0;
		return zero;
	}
	return m_carrying->getMass();
}
Speed CanPickup::speedIfCarryingQuantity(const HasShape& hasShape, Quantity quantity) const
{
	assert(m_carrying == nullptr);
	return m_actor.m_canMove.getIndividualMoveSpeedWithAddedMass(quantity * hasShape.singleUnitMass());
}
Quantity CanPickup::maximumNumberWhichCanBeCarriedWithMinimumSpeed(const HasShape& hasShape, Speed minimumSpeed) const
{
	assert(minimumSpeed != 0);
	//TODO: replace iteration with calculation?
	Quantity quantity = 0;
	while(speedIfCarryingQuantity(hasShape, quantity + 1) >= minimumSpeed && quantity < hasShape.getQuantity())
		quantity++;
	return quantity;
}
HaulSubproject::HaulSubproject(Project& p, HaulSubprojectParamaters& paramaters) : m_project(p), m_workers(paramaters.workers.begin(), paramaters.workers.end()), m_toHaul(*paramaters.toHaul), m_quantity(paramaters.quantity), m_strategy(paramaters.strategy), m_haulTool(paramaters.haulTool), m_leader(nullptr), m_itemIsMoving(false), m_beastOfBurden(paramaters.beastOfBurden), m_projectRequirementCounts(*paramaters.projectRequirementCounts), m_genericItemType(nullptr), m_genericMaterialType(nullptr)
{ 
	assert(!m_workers.empty());
	if(m_haulTool != nullptr)
	{
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		m_haulTool->m_reservable.reserveFor(m_project.m_canReserve, 1u, std::move(dishonorCallback));
	}
	if(m_beastOfBurden != nullptr)
	{
		std::unique_ptr<DishonorCallback> dishonorCallback = std::make_unique<HaulSubprojectDishonorCallback>(*this);
		m_beastOfBurden->m_reservable.reserveFor(m_project.m_canReserve, 1u, std::move(dishonorCallback));
	}
	if(m_toHaul.isGeneric())
	{
		m_genericItemType = &static_cast<Item&>(m_toHaul).m_itemType;
		m_genericMaterialType = &static_cast<Item&>(m_toHaul).m_materialType;
	}
	for(Actor* actor : m_workers)
	{
		m_project.m_workers.at(actor).haulSubproject = this;
		commandWorker(*actor);
	}
}
HaulSubproject::HaulSubproject(const Json& data, Project& p, DeserializationMemo& deserializationMemo) : m_project(p), 
	m_toHaul(data.contains("toHaulItem") ? static_cast<HasShape&>(deserializationMemo.itemReference(data["toHaulItem"])) : static_cast<HasShape&>(deserializationMemo.actorReference(data["toHaulActor"]))),
	m_quantity(data["quantity"].get<Quantity>()), 
	m_strategy(haulStrategyFromName(data["haulStrategy"].get<std::string>())),
	m_haulTool(nullptr), 
	m_leader(nullptr), 
	m_itemIsMoving(data["itemIsMoving"].get<bool>()), 
	m_beastOfBurden(nullptr), 
	m_projectRequirementCounts(deserializationMemo.projectRequirementCountsReference(data["requirementCounts"])),
       	m_genericItemType(nullptr), 
	m_genericMaterialType(nullptr)
{ 
	if(data.contains("haulTool"))
		m_haulTool = &deserializationMemo.m_simulation.m_hasItems->getById(data["haulTool"].get<ItemId>());
	if(data.contains("leader"))
		m_leader = &deserializationMemo.m_simulation.m_hasActors->getById(data["leader"].get<ActorId>());
	if(data.contains("beastOfBurden"))
		m_beastOfBurden = &deserializationMemo.m_simulation.m_hasActors->getById(data["beastOfBurden"].get<ActorId>());
	if(data.contains("genericItemType"))
		m_genericItemType = &ItemType::byName(data["genericItemType"].get<std::string>());
	if(data.contains("genericMaterialType"))
		m_genericMaterialType = &MaterialType::byName(data["genericMaterialType"].get<std::string>());
	if(data.contains("workers"))
		for(const Json& workerId : data["workers"])
			m_workers.insert(&deserializationMemo.m_simulation.m_hasActors->getById(workerId.get<ActorId>()));
	if(data.contains("nonsentients"))
		for(const Json& actorId : data["nonsentients"])
			m_nonsentients.insert(&deserializationMemo.m_simulation.m_hasActors->getById(actorId.get<ActorId>()));
	if(data.contains("liftPoints"))
		for(const Json& liftPointData : data["liftPoints"])
		{
			Actor& actor = deserializationMemo.m_simulation.m_hasActors->getById(liftPointData[0].get<ActorId>());
			m_liftPoints[&actor] = deserializationMemo.m_simulation.getBlockForJsonQuery(liftPointData[1]);
		}
	deserializationMemo.m_haulSubprojects[data["address"].get<uintptr_t>()] = this;
}
Json HaulSubproject::toJson() const
{
	Json data({
		{"quantity", m_quantity},
		{"haulStrategy", haulStrategyToName(m_strategy)},
		{"itemIsMoving", m_itemIsMoving},
		{"address", reinterpret_cast<uintptr_t>(this)},
		{"requirementCounts", reinterpret_cast<uintptr_t>(&m_projectRequirementCounts)},
	});
	if(m_toHaul.isItem())
		data["toHaulItem"] = static_cast<Item&>(m_toHaul).m_id;
	else
	{
		assert(m_toHaul.isActor());
		data["toHaulActor"] = static_cast<Actor&>(m_toHaul).m_id;
	}
	if(m_haulTool != nullptr)
		data["haulTool"] = m_haulTool->m_id;
	if(m_leader != nullptr)
		data["leader"] = m_leader->m_id;
	if(m_beastOfBurden != nullptr)
		data["beastOfBurden"] = m_beastOfBurden->m_id;
	if(m_genericItemType != nullptr)
		data["genericItemType"] = m_genericItemType->name;
	if(m_genericMaterialType!= nullptr)
		data["genericMaterialType"] = m_genericMaterialType->name;
	if(!m_workers.empty())
	{
		data["workers"] = Json::array();
		for(Actor* worker : m_workers)
			data["workers"].push_back(worker->m_id);
	}
	if(!m_nonsentients.empty())
	{
		data["nonsentients"] = Json::array();
		for(Actor* nonsentient : m_nonsentients)
			data["nonsentients"].push_back(nonsentient->m_id);
	}
	if(!m_liftPoints.empty())
	{
		data["liftPoints"] = Json::array();
		for(auto& [actor, block] : m_liftPoints)
			data["liftPoints"].push_back(Json::array({actor->m_id, block}));
	}
	return data;
}
void HaulSubproject::commandWorker(Actor& actor)
{
	assert(m_workers.contains(&actor));
	assert(actor.m_hasObjectives.hasCurrent());
	auto& blocks = actor.m_area->getBlocks();
	Objective& objective = m_project.m_workers.at(&actor).objective;
	bool detour = objective.m_detour;
	bool hasCargo = false;
	switch(m_strategy)
	{
		case HaulStrategy::Individual:
			assert(m_workers.size() == 1);
			hasCargo = m_genericItemType != nullptr ? 
				actor.m_canPickup.isCarryingGeneric(*m_genericItemType, *m_genericMaterialType, m_quantity) :
				actor.m_canPickup.isCarrying(m_toHaul);
			if(hasCargo)
			{
				if(actor.isAdjacentTo(m_project.m_location))
				{
					// Unload
					HasShape& cargo = actor.m_canPickup.putDown(actor.m_location, m_quantity);
					complete(cargo);
				}
				else
					actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
			}
			else
			{
				if(actor.isAdjacentTo(m_toHaul))
				{
					actor.m_canReserve.deleteAllWithoutCallback();
					m_toHaul.m_reservable.clearReservationFor(m_project.m_canReserve, m_quantity);
					actor.m_canPickup.pickUp(m_toHaul, m_quantity);
					// From here on out we cannot use m_toHaul unless we test for generic.
					commandWorker(actor);
				}
				else
					actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
			}
			break;
		case HaulStrategy::IndividualCargoIsCart:
			assert(m_workers.size() == 1);
			if(actor.m_canLead.isLeading(m_toHaul))
			{
				if(actor.isAdjacentTo(m_project.m_location))
				{
					// Unload
					m_toHaul.m_canFollow.unfollow();
					complete(m_toHaul);
				}
				else
					actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
			}
			else
			{
				if(actor.isAdjacentTo(m_toHaul))
				{
					actor.m_canReserve.deleteAllWithoutCallback();
					m_toHaul.m_reservable.clearReservationFor(m_project.m_canReserve, m_quantity);
					m_toHaul.m_canFollow.follow(actor.m_canLead, true);
					commandWorker(actor);
				}
				else
					actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
			}
			break;
		case HaulStrategy::Team:
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(m_leader == nullptr)
				m_leader = &actor;
			if(m_itemIsMoving)
			{
				assert(&actor == m_leader);
				if(actor.isAdjacentTo(m_project.m_location))
				{
					// At destination.
					m_toHaul.m_canFollow.unfollow();
					for(Actor* actor : m_workers)
						if(actor != m_leader)
							actor->m_canFollow.unfollow();
					m_toHaul.setLocation(actor.m_location, actor.m_area);
					complete(m_toHaul);
				}
				else
					actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
			}
			else 
			{
				// Item is not moving.
				if(m_liftPoints.contains(&actor))
				{
					// A lift point exists.
					if(actor.m_location == m_liftPoints.at(&actor))
					{
						// Actor is at lift point.
						if(allWorkersAreAdjacentTo(m_toHaul))
						{
							// All actors are at lift points.
							m_toHaul.m_canFollow.follow(m_leader->m_canLead);
							for(Actor* follower : m_workers)
							{
								if(follower != m_leader)
									follower->m_canFollow.follow(m_toHaul.m_canLead);
								follower->m_canReserve.deleteAllWithoutCallback();
							}
							m_leader->m_canReserve.deleteAllWithoutCallback();
							m_leader->m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
							m_itemIsMoving = true;
							m_toHaul.m_reservable.clearReservationFor(m_project.m_canReserve, m_quantity);
						}
					}
					else
						// Actor is not at lift point.
						// destination, detour, adjacent, unreserved, reserve
						actor.m_canMove.setDestination(m_liftPoints.at(&actor), detour, false, false, false);
				}
				else
				{
					// No lift point exists for this actor, find one.
					// TODO: move this logic to tryToSetHaulStrategy?
					for(BlockIndex block : m_toHaul.getAdjacentBlocks())
					{
						Facing facing = blocks.facingToSetWhenEnteringFrom(m_project.m_location, block);
						if(blocks.shape_canEnterEverWithFacing(block, actor, facing) && !blocks.isReserved(block, *actor.getFaction()))
						{
							m_liftPoints[&actor] = block;
							blocks.reserve(block, actor.m_canReserve);
							// Destination, detour, adjacent, unreserved, reserve
							actor.m_canMove.setDestination(block, detour, false, false, false);
							return;
						}
					}
					assert(false); // team strategy should not be choosen if there is not enough space to reserve for lifting.
				}
			}
			break;
			//TODO: Reserve destinations?
		case HaulStrategy::Cart:
			assert(m_haulTool != nullptr);
			if(actor.m_canLead.isLeading(*m_haulTool))
			{
				// Has cart.
				hasCargo = m_genericItemType != nullptr ? 
					m_haulTool->m_hasCargo.containsGeneric(*m_genericItemType, *m_genericMaterialType, m_quantity) :
					m_haulTool->m_hasCargo.contains(m_toHaul);
				if(hasCargo)
				{
					// Cart is loaded.
					if(actor.isAdjacentTo(m_project.m_location))
					{
						// At drop off point.
						m_haulTool->m_canFollow.unfollow();
						HasShape* delivered = nullptr;
						if(m_genericItemType == nullptr)
						{
							m_haulTool->m_hasCargo.unloadTo(m_toHaul, m_project.m_location);
							delivered = &m_toHaul;
						}
						else
							delivered = &m_haulTool->m_hasCargo.unloadGenericTo(*m_genericItemType, *m_genericMaterialType, m_quantity, actor.m_location);
						// TODO: set rotation?
						assert(delivered != nullptr);
						complete(*delivered);
					}
					else
						// Go to drop off point.
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
				}
				else
				{
					// Cart not loaded.
					if(actor.isAdjacentTo(m_toHaul))
					{
						// Can load here.
						//TODO: set delay for loading.
						m_toHaul.m_reservable.maybeClearReservationFor(m_project.m_canReserve, m_quantity);
						m_haulTool->m_hasCargo.load(m_toHaul, m_quantity);
						actor.m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
					}
					else
						// Go somewhere to load.
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
				}
			}
			else
			{
				// Don't have Cart.
				if(actor.isAdjacentTo(*m_haulTool))
				{
					// Cart is here.
					m_haulTool->m_canFollow.follow(actor.m_canLead);
					actor.m_canReserve.deleteAllWithoutCallback();
					actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
				}
				else
					// Go to cart.
					actor.m_canMove.setDestinationAdjacentTo(*m_haulTool, detour);

			}
			break;
		case HaulStrategy::Panniers:
			assert(m_beastOfBurden != nullptr);
			assert(m_haulTool != nullptr);
			if(m_beastOfBurden->m_equipmentSet.contains(*m_haulTool))
			{
				// Beast has panniers.
				hasCargo = m_genericItemType != nullptr ? 
					m_haulTool->m_hasCargo.containsGeneric(*m_genericItemType, *m_genericMaterialType, m_quantity) :
					m_haulTool->m_hasCargo.contains(m_toHaul);
				if(hasCargo)
				{
					// Panniers have cargo.
					if(actor.isAdjacentTo(m_project.m_location))
					{
						// Actor is at destination.
						HasShape* delivered = nullptr;
						//TODO: unloading delay.
						if(m_genericItemType == nullptr)
						{
							m_haulTool->m_hasCargo.unloadTo(m_toHaul, actor.m_location);
							delivered = &m_toHaul;
						}
						else
						{
							delivered = &m_haulTool->m_hasCargo.unloadGenericTo(*m_genericItemType, *m_genericMaterialType, m_quantity, actor.m_location);
						}
						// TODO: set rotation?
						m_beastOfBurden->m_canFollow.unfollow();
						complete(*delivered);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
				}
				else
				{
					// Panniers don't have cargo.
					if(actor.isAdjacentTo(m_toHaul))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						m_toHaul.m_reservable.maybeClearReservationFor(m_project.m_canReserve, m_quantity);
						m_haulTool->m_hasCargo.load(m_toHaul, m_quantity);
						actor.m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
				}
			}
			else
			{
				// Get beast panniers.
				if(actor.m_canPickup.isCarrying(*m_haulTool))
				{
					// Actor has panniers.
					if(actor.isAdjacentTo(*m_beastOfBurden))
					{
						// Actor can put on panniers.
						actor.m_canPickup.remove(*m_haulTool);
						m_beastOfBurden->m_equipmentSet.addEquipment(*m_haulTool);
						m_beastOfBurden->m_canFollow.follow(actor.m_canLead);
						actor.m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_beastOfBurden, detour);
				}
				else
				{
					// Bring panniers to beast.
					// TODO: move to adjacent to panniers.
					if(actor.isAdjacentTo(m_haulTool->m_location))
					{
						actor.m_canPickup.pickUp(*m_haulTool, 1u);
						actor.m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(*m_beastOfBurden, detour);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_haulTool, detour);
				}

			}
			break;
		case HaulStrategy::AnimalCart:
			//TODO: what if already attached to a different cart?
			assert(m_beastOfBurden != nullptr);
			assert(m_haulTool != nullptr);
			if(m_beastOfBurden->m_canLead.isLeading(*m_haulTool))
			{
				// Beast has cart.
				hasCargo = m_genericItemType != nullptr ? 
					m_haulTool->m_hasCargo.containsGeneric(*m_genericItemType, *m_genericMaterialType, m_quantity) :
					m_haulTool->m_hasCargo.contains(m_toHaul);
				if(hasCargo)
				{
					// Cart has cargo.
					if(actor.isAdjacentTo(m_project.m_location))
					{
						// Actor is at destination.
						//TODO: unloading delay.
						//TODO: unfollow cart?
						HasShape* delivered = nullptr;
						if(m_genericItemType == nullptr)
						{
							m_haulTool->m_hasCargo.unloadTo(m_toHaul, actor.m_location);
							delivered = &m_toHaul;
						}
						else
							delivered = &m_haulTool->m_hasCargo.unloadGenericTo(*m_genericItemType, *m_genericMaterialType, m_quantity, actor.m_location);
						// TODO: set rotation?
						m_beastOfBurden->m_canFollow.unfollow();
						complete(*delivered);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
				}
				else
				{
					// Cart doesn't have cargo.
					if(actor.isAdjacentTo(m_toHaul))
					{
						// Actor is at pickup location.
						// TODO: loading delay.
						m_toHaul.m_reservable.maybeClearReservationFor(m_project.m_canReserve);
						m_haulTool->m_hasCargo.load(m_toHaul, m_quantity);
						actor.m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
				}
			}
			else
			{
				// Bring beast to cart.
				if(actor.m_canLead.isLeading(*m_beastOfBurden))
				{
					// Actor has beast.
					if(actor.isAdjacentTo(*m_haulTool))
					{
						// Actor can harness beast to item.
						// Don't check if item is adjacent to beast, allow it to teleport.
						// TODO: Make not teleport.
						m_haulTool->m_canFollow.follow(m_beastOfBurden->m_canLead, false);
						// Skip adjacent check, potentially teleport.
						actor.m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_haulTool, detour);
				}
				else
				{
					// Get beast.
					if(actor.isAdjacentTo(*m_beastOfBurden))
					{
						m_beastOfBurden->m_canFollow.follow(actor.m_canLead);
						actor.m_canReserve.deleteAllWithoutCallback();
						m_beastOfBurden->m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(*m_haulTool, detour);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(*m_beastOfBurden, detour);
				}

			}
			break;
		case HaulStrategy::TeamCart:
			assert(m_haulTool != nullptr);
			// TODO: teams of more then 2, requires muliple followers.
			assert(m_workers.size() == 2);
			if(m_leader == nullptr)
				m_leader = &actor;
			if(actor.m_canLead.isLeading(*m_haulTool))
			{
				assert(&actor == m_leader);
				hasCargo = m_genericItemType != nullptr ? 
					m_haulTool->m_hasCargo.containsGeneric(*m_genericItemType, *m_genericMaterialType, m_quantity) :
					m_haulTool->m_hasCargo.contains(m_toHaul);
				if(hasCargo)
				{
					if(actor.isAdjacentTo(m_project.m_location))
					{
						HasShape* delivered = nullptr;
						if(m_genericItemType == nullptr)
						{
							m_haulTool->m_hasCargo.unloadTo(m_toHaul, actor.m_location);
							delivered = &m_toHaul;
						}
						else
							delivered = &m_haulTool->m_hasCargo.unloadGenericTo(*m_genericItemType, *m_genericMaterialType, m_quantity, actor.m_location);
						// TODO: set rotation?
						m_leader->m_canFollow.disband();
						complete(*delivered);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
				}
				else
				{
					if(actor.isAdjacentTo(m_toHaul))
					{
						//TODO: set delay for loading.
						m_toHaul.m_reservable.maybeClearReservationFor(m_project.m_canReserve);
						m_haulTool->m_hasCargo.load(m_toHaul, m_quantity);
						actor.m_canReserve.deleteAllWithoutCallback();
						actor.m_canMove.setDestinationAdjacentTo(m_project.m_location, detour);
					}
					else
						actor.m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
				}
			}
			else if(actor.isAdjacentTo(*m_haulTool))
			{
				if(allWorkersAreAdjacentTo(*m_haulTool))
				{
					// All actors are adjacent to the haul tool.
					m_haulTool->m_canFollow.follow(m_leader->m_canLead);
					for(Actor* follower : m_workers)
					{
						if(follower != m_leader)
							follower->m_canFollow.follow(m_haulTool->m_canLead);
						follower->m_canReserve.deleteAllWithoutCallback();
					}
					m_leader->m_canReserve.deleteAllWithoutCallback();
					m_leader->m_canMove.setDestinationAdjacentTo(m_toHaul, detour);
				} 
			}
			else
			{
				actor.m_canReserve.deleteAllWithoutCallback();
				actor.m_canMove.setDestinationAdjacentTo(*m_haulTool, detour);
			}
			break;
		case HaulStrategy::StrongSentient:
			//TODO
			break;
		case HaulStrategy::None:
			assert(false); // this method should only be called after a strategy is choosen.
	}
}
void HaulSubproject::addWorker(Actor& actor)
{
	assert(!m_workers.contains(&actor));
	m_workers.insert(&actor);
	commandWorker(actor);
}
void HaulSubproject::removeWorker([[maybe_unused]] Actor& actor)
{
	assert(m_workers.contains(&actor));
	cancel();
}
void HaulSubproject::cancel()
{
	m_project.haulSubprojectCancel(*this);
}
bool HaulSubproject::allWorkersAreAdjacentTo(HasShape& hasShape)
{
	return std::all_of(m_workers.begin(), m_workers.end(), [&](Actor* worker) { return worker->isAdjacentTo(hasShape); });
}
HaulSubprojectParamaters HaulSubproject::tryToSetHaulStrategy(const Project& project, HasShape& toHaul, Actor& worker)
{
	// TODO: make exception for slow haul if very close.
	HaulSubprojectParamaters output;
	output.toHaul = &toHaul;
	output.projectRequirementCounts = project.m_toPickup.at(&toHaul).first;
	Quantity maxQuantityRequested = project.m_toPickup.at(&toHaul).second;
	Speed minimumSpeed = project.getMinimumHaulSpeed();
	std::vector<Actor*> workers;
	// TODO: shouldn't this be m_waiting?
	for(auto& pair : project.m_workers)
		workers.push_back(pair.first);
	assert(maxQuantityRequested != 0);
	// toHaul is wheeled.
	static const MoveType& wheeled = MoveType::byName("roll");
	if(toHaul.getMoveType() == wheeled)
	{
		assert(toHaul.isItem());
		Item& haulableItem = static_cast<Item&>(toHaul);
		std::vector<const HasShape*> list{&worker, &toHaul};
		if(CanLead::getMoveSpeedForGroup(list) >= minimumSpeed)
		{
			output.strategy = HaulStrategy::IndividualCargoIsCart;
			output.quantity = 1;
			output.workers.push_back(&worker);
			return output;
		}
		else
		{
			output.workers = actorsNeededToHaulAtMinimumSpeedWithTool(project, worker, toHaul, haulableItem);
			if(!output.workers.empty())
			{
				assert(output.workers.size() > 1);
				output.strategy = HaulStrategy::Team;
				output.quantity = 1;
				return output;
			}
		}
	}
	// Individual
	// TODO:: Prioritize cart if a large number of items are requested.
	Quantity maxQuantityCanCarry = worker.m_canPickup.maximumNumberWhichCanBeCarriedWithMinimumSpeed(toHaul, minimumSpeed);
	if(maxQuantityCanCarry != 0)
	{
		output.strategy = HaulStrategy::Individual;
		output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
		output.workers.push_back(&worker);
		return output;
	}
	// Team
	output.workers = actorsNeededToHaulAtMinimumSpeed(project, worker, toHaul);
	if(!output.workers.empty())
	{
		assert(output.workers.size() == 2); // TODO: teams larger then two.
		output.strategy = HaulStrategy::Team;
		output.quantity = 1;
		return output;
	}
	Item* haulTool = worker.m_area->m_hasHaulTools.getToolToHaul(*worker.getFaction(), toHaul);
	// Cart
	if(haulTool != nullptr)
	{
		// Cart
		maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(worker, *haulTool, toHaul, minimumSpeed);
		if(maxQuantityCanCarry != 0)
		{
			output.strategy = HaulStrategy::Cart;
			output.haulTool = haulTool;
			output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
			output.workers.push_back(&worker);
			return output;
		}
		// Animal Cart
		Actor* yoked = worker.m_area->m_hasHaulTools.getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(*worker.getFaction(), *haulTool, toHaul.getMass(), minimumSpeed);
		if(yoked != nullptr)
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(worker, *yoked, *haulTool, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::AnimalCart;
				output.haulTool = haulTool;
				output.beastOfBurden = yoked;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.push_back(&worker);
				return output;
			}
		}
		// Team Cart
		output.workers = actorsNeededToHaulAtMinimumSpeedWithTool(project, worker, toHaul, *haulTool);
		if(!output.workers.empty())
		{
			assert(output.workers.size() > 1);
			output.strategy = HaulStrategy::TeamCart;
			output.haulTool = haulTool;
			output.quantity = 1;
			return output;
		}
	}
	// Panniers
	Actor* pannierBearer = worker.m_area->m_hasHaulTools.getPannierBearerToHaulCargoWithMassWithMinimumSpeed(*worker.getFaction(), toHaul, minimumSpeed);
	if(pannierBearer != nullptr)
	{
		//TODO: If pannierBearer already has panniers equiped then use those, otherwise find ones to use. Same for animalCart.
		Item* panniers = worker.m_area->m_hasHaulTools.getPanniersForActorToHaul(*worker.getFaction(), *pannierBearer, toHaul);
		if(panniers != nullptr)
		{
			maxQuantityCanCarry = maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(worker, *pannierBearer, *panniers, toHaul, minimumSpeed);
			if(maxQuantityCanCarry != 0)
			{
				output.strategy = HaulStrategy::Panniers;
				output.beastOfBurden = pannierBearer;
				output.haulTool = panniers;
				output.quantity = std::min(maxQuantityRequested, maxQuantityCanCarry);
				output.workers.push_back(&worker);
				return output;
			}
		}
	}
	assert(output.strategy == HaulStrategy::None);
	return output;
}
void HaulSubproject::complete(HasShape& delivered)
{
	if(m_haulTool != nullptr)
		m_haulTool->m_reservable.clearReservationFor(m_project.m_canReserve);
	if(m_beastOfBurden != nullptr)
		m_beastOfBurden->m_reservable.clearReservationFor(m_project.m_canReserve);
	m_projectRequirementCounts.delivered += m_quantity;
	assert(m_projectRequirementCounts.delivered <= m_projectRequirementCounts.required);
	if(delivered.isItem())
	{
		if(m_projectRequirementCounts.consumed)
			m_project.m_toConsume.insert(static_cast<Item*>(&delivered));
		if(static_cast<Item&>(delivered).isWorkPiece())
			delivered.setLocation(m_project.m_location, &m_project.m_area);
	}
	for(Actor* worker : m_workers)
		worker->m_canReserve.deleteAllWithoutCallback();
	delivered.m_reservable.reserveFor(m_project.m_canReserve, m_quantity);
	std::vector<Actor*> workers(m_workers.begin(), m_workers.end());
	Project& project = m_project;
	m_project.m_deliveredItems.push_back(static_cast<Item*>(&delivered));
	m_project.onDelivered(delivered);
	m_project.m_haulSubprojects.remove(*this);
	for(Actor* worker : workers)
	{
		project.m_workers.at(worker).haulSubproject = nullptr;
		project.commandWorker(*worker);
	}
}
// Class method.
std::vector<Actor*> HaulSubproject::actorsNeededToHaulAtMinimumSpeed(const Project& project, Actor& leader, const HasShape& toHaul)
{
	std::vector<const HasShape*> actorsAndItems;
	std::vector<Actor*> output;
	output.push_back(&leader);
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&toHaul);
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		if(actor == &leader || projectWorker.haulSubproject != nullptr || !actor->isSentient())
			continue;
		assert(std::ranges::find(output, actor) == output.end());
		output.push_back(actor);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(actor);
		Speed speed = CanLead::getMoveSpeedForGroup(actorsAndItems);
		if(speed >= project.getMinimumHaulSpeed())
			return output;
	}
	// Cannot form a team to move at acceptable speed.
	output.clear();
	return output;
}
//Class method.
Speed HaulSubproject::getSpeedWithHaulToolAndCargo(const Actor& leader, const Item& haulTool, const HasShape& toHaul, Quantity quantity)
{
	std::vector<const HasShape*> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&haulTool);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, toHaul.singleUnitMass() * quantity, 0);
}
// Class method.
Quantity HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithTool(const Actor& leader, const Item& haulTool, const HasShape& toHaul, Speed minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = 0;
	while(getSpeedWithHaulToolAndCargo(leader, haulTool, toHaul, quantity + 1) >= minimumSpeed)
		quantity++;
	return quantity;
}
// Class method.
Speed HaulSubproject::getSpeedWithHaulToolAndAnimal(const Actor& leader, const Actor& yoked, const Item& haulTool, const HasShape& toHaul, Quantity quantity)
{
	std::vector<const HasShape*> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&yoked);
	actorsAndItems.push_back(&haulTool);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, toHaul.singleUnitMass() * quantity, 0);
}
// Class method.
Speed HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithToolAndAnimal(const Actor& leader, Actor& yoked, const Item& haulTool, const HasShape& toHaul, Speed minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = 0;
	while(getSpeedWithHaulToolAndAnimal(leader, yoked, haulTool, toHaul, quantity + 1) >= minimumSpeed)
		quantity++;
	return quantity;
}
// Class method.
std::vector<Actor*> HaulSubproject::actorsNeededToHaulAtMinimumSpeedWithTool(const Project& project, Actor& leader, const HasShape& toHaul, const Item& haulTool)
{
	std::vector<const HasShape*> actorsAndItems;
	std::vector<Actor*> output;
	output.push_back(&leader);
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&haulTool);
	// For each actor without a sub project add to actors and items and see if the item can be moved fast enough.
	for(auto& [actor, projectWorker] : project.m_workers)
	{
		if(actor == &leader || projectWorker.haulSubproject != nullptr || !actor->isSentient())
			continue;
		assert(std::ranges::find(output, actor) == output.end());
		output.push_back(actor);
		if(output.size() > 2) //TODO: More then two requires multiple followers for one leader.
			break;
		actorsAndItems.push_back(actor);
		// If to haul is also haul tool then there is no cargo, we are hauling the haulTool itself.
		Mass mass = &toHaul == &static_cast<const HasShape&>(haulTool) ? 0 : toHaul.singleUnitMass();
		Speed speed = CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, mass);
		if(speed >= project.getMinimumHaulSpeed())
			return output;
	}
	// Cannot form a team to move at acceptable speed.
	output.clear();
	return output;
}
// Class method.
Speed HaulSubproject::getSpeedWithPannierBearerAndPanniers(const Actor& leader, const Actor& pannierBearer, const Item& panniers, const HasShape& toHaul, Quantity quantity)
{
	std::vector<const HasShape*> actorsAndItems;
	actorsAndItems.push_back(&leader);
	actorsAndItems.push_back(&pannierBearer);
	// actorsAndItems, rollingMass, deadMass
	return CanLead::getMoveSpeedForGroupWithAddedMass(actorsAndItems, 0, (toHaul.singleUnitMass() * quantity) + panniers.getMass());
}
// Class method.
Quantity HaulSubproject::maximumNumberWhichCanBeHauledAtMinimumSpeedWithPanniersAndAnimal(const Actor& leader, const Actor& pannierBearer, const Item& panniers, const HasShape& toHaul, Speed minimumSpeed)
{
	assert(minimumSpeed != 0);
	Quantity quantity = 0;
	while(getSpeedWithPannierBearerAndPanniers(leader, pannierBearer, panniers, toHaul, quantity + 1) >= minimumSpeed)
		quantity++;
	return quantity;
}
//TODO: optimize?
bool AreaHasHaulTools::hasToolToHaul(Faction& faction, const HasShape& hasShape) const
{
	return getToolToHaul(faction, hasShape) != nullptr;
}
Item* AreaHasHaulTools::getToolToHaul(Faction& faction, const HasShape& hasShape) const
{
	// Items like panniers also have internal volume but aren't relevent for this method.
	static const MoveType& none = MoveType::byName("none");
	for(Item* item : m_haulTools)
		if(item->m_itemType.moveType != none && !item->m_reservable.isFullyReserved(&faction) && item->m_itemType.internalVolume >= hasShape.getVolume())
			return item;
	return nullptr;
}
bool AreaHasHaulTools::hasToolToHaulFluid(Faction& faction) const
{
	return getToolToHaulFluid(faction) != nullptr;
}
Item* AreaHasHaulTools::getToolToHaulFluid(Faction& faction) const
{
	for(Item* item : m_haulTools)
		if(!item->m_reservable.isFullyReserved(&faction) && item->m_itemType.canHoldFluids)
			return item;
	return nullptr;
}
Actor* AreaHasHaulTools::getActorToYokeForHaulToolToMoveCargoWithMassWithMinimumSpeed(Faction& faction, const Item& haulTool, Mass cargoMass, Speed minimumHaulSpeed) const
{
	[[maybe_unused]] static const MoveType& rollingType = MoveType::byName("roll");
	assert(haulTool.m_itemType.moveType == rollingType);
	for(Actor* actor : m_yolkableActors)
	{
		std::vector<const HasShape*> shapes = { actor, &haulTool };
		if(!actor->m_reservable.isFullyReserved(&faction) && minimumHaulSpeed <= actor->m_canLead.getMoveSpeedForGroupWithAddedMass(shapes, cargoMass))
			return actor;
	}
	return nullptr;
}
Actor* AreaHasHaulTools::getPannierBearerToHaulCargoWithMassWithMinimumSpeed(Faction& faction, const HasShape& hasShape, Speed minimumHaulSpeed) const
{
	//TODO: Account for pannier mass?
	for(Actor* actor : m_yolkableActors)
		if(!actor->m_reservable.isFullyReserved(&faction) && minimumHaulSpeed <= actor->m_canPickup.speedIfCarryingQuantity(hasShape, 1u))
			return actor;
	return nullptr;
}
Item* AreaHasHaulTools::getPanniersForActorToHaul(Faction& faction, const Actor& actor, const HasShape& toHaul) const
{
	for(Item* item : m_haulTools)
		if(!item->m_reservable.isFullyReserved(&faction) && item->m_itemType.internalVolume >= toHaul.getVolume() && actor.m_equipmentSet.canEquipCurrently(*item))
			return item;
	return nullptr;
}
void AreaHasHaulTools::registerHaulTool(Item& item)
{
	assert(!m_haulTools.contains(&item));
	assert(item.m_itemType.internalVolume != 0);
	m_haulTools.insert(&item);
}
void AreaHasHaulTools::registerYokeableActor(Actor& actor)
{
	assert(!m_yolkableActors.contains(&actor));
	m_yolkableActors.insert(&actor);
}
void AreaHasHaulTools::unregisterHaulTool(Item& item)
{
	assert(m_haulTools.contains(&item));
	m_haulTools.erase(&item);
}
void AreaHasHaulTools::unregisterYokeableActor(Actor& actor)
{
	assert(m_yolkableActors.contains(&actor));
	m_yolkableActors.erase(&actor);
}
