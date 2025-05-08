#include "deck.h"
#include "area/area.h"
#include "blocks/blocks.h"
#include "actors/actors.h"
#include "items/items.h"
#include "reservable.hpp"
AreaHasDecks::AreaHasDecks(Area& area)
{
	m_blockData.resize(area.getBlocks().size());
}
void AreaHasDecks::updateBlocks(Area& area, const DeckId& id)
{
	auto view = m_data[id].cuboidSet.getView(area.getBlocks());
	for(const BlockIndex& block : view)
	{
		assert(m_blockData[block].empty());
		m_blockData[block] = id;
		// TODO: bulk update.
		area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
	}
}
void AreaHasDecks::clearBlocks(Area& area, const DeckId& id)
{
	for(const BlockIndex& block : m_data[id].cuboidSet.getView(area.getBlocks()))
	{
		m_blockData[block].clear();
		// TODO: bulk update.
		area.m_hasTerrainFacades.updateBlockAndAdjacent(block);
	}
}
[[nodiscard]] DeckId AreaHasDecks::registerDecks(Area& area, CuboidSet& decks, const ActorOrItemIndex& actorOrItemIndex)
{
	m_data.emplace(m_nextId, CuboidSetAndActorOrItemIndex{decks, actorOrItemIndex});
	updateBlocks(area, m_nextId);
	auto output = m_nextId;
	++m_nextId;
	return output;
}
void AreaHasDecks::unregisterDecks(Area& area, const DeckId& id)
{
	clearBlocks(area, id);
	m_data.erase(id);
}
void AreaHasDecks::shift(Area& area, const DeckId& id, const Offset3D& offset, const DistanceInBlocks& distance, const BlockIndex& origin, const Facing4& oldFacing, const Facing4& newFacing)
{
	clearBlocks(area, id);
	if(oldFacing != newFacing)
	{
		int facingChange = (int)oldFacing - (int)newFacing;
		if(facingChange < 0)
			facingChange += 4;
		m_data[id].cuboidSet.rotateAroundPoint(area.getBlocks(), origin, (Facing4)facingChange);
	}
	m_data[id].cuboidSet.shift(offset, distance);
	updateBlocks(area, id);
}
DeckRotationData DeckRotationData::recordAndClearDependentPositions(Area& area, const ActorOrItemIndex& actorOrItem)
{
	DeckRotationData output;
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	Blocks& blocks = area.getBlocks();
	SmallSet<ActorOrItemIndex> actorsOrItemsOnDeck;
	SmallSet<Project*> projectsOnDeck;
	SmallSet<BlockIndex> blocksContainingFluid;
	if(actorOrItem.isActor())
	{
		const ActorIndex& actor = actorOrItem.getActor();
		actorsOrItemsOnDeck = actors.onDeck_get(actor);
	}
	else
	{
		const ItemIndex& item = actorOrItem.getItem();
		actorsOrItemsOnDeck = items.onDeck_get(item);
		projectsOnDeck = items.onDeck_getProjects(item);
		blocksContainingFluid = items.onDeck_getBlocksContainingFluid(item);
	}
	for(const ActorOrItemIndex& onDeck : actorsOrItemsOnDeck)
	{
		if(onDeck.isActor())
		{
			ActorIndex actor = onDeck.getActor();
			// TODO: this fetches the DeckId seperately for each actor but they are all the same.
			DeckRotationDataSingle data{actors.canReserve_unreserveAndReturnBlocksAndCallbacksOnSameDeck(actor), actors.getLocation(actor), actors.getFacing(actor)};
			output.m_actorsOrItems.insert(onDeck, data);
			actors.location_clear(actor);
		}
		else
		{
			const ItemIndex& item = onDeck.getItem();
			output.m_actorsOrItems.insert(onDeck, DeckRotationDataSingle{{}, items.getLocation(item), items.getFacing(item)});
			items.location_clear(item);
		}
	}
	if(actorOrItem.isItem())
	{
		const ItemIndex& item = actorOrItem.getItem();
		if(items.onDeck_hasDecks(item))
		{
			const DeckId& deckId = area.m_decks.getForBlock(items.getLocation(item));
			if(deckId.exists())
			{
				for(Project* project : projectsOnDeck)
				{
					auto condition = [&](const BlockIndex& block) { return area.m_decks.getForBlock(block) == deckId; };
					output.m_projects.insert(project, {project->getCanReserve().unreserveAndReturnBlocksAndCallbacksWithCondition(condition), project->getLocation(), Facing4::Null});
					project->clearLocation();
				}
				for(const BlockIndex& block : blocksContainingFluid)
				{
					output.m_fluids.insert(block, {});
					auto& storedData = output.m_fluids.back().second;
					for(const FluidData& fluidData : blocks.fluid_getAll(block))
						storedData.insert(const_cast<FluidTypeId&>(fluidData.type), const_cast<CollisionVolume&>(fluidData.volume));
					blocks.fluid_removeAllSyncronus(block);
				}
			}
		}
	}
	return output;
}
void DeckRotationData::reinstanceAtRotatedPosition(Area& area, const BlockIndex& previousPivot, const BlockIndex& newPivot, const Facing4& previousFacing, const Facing4& newFacing)
{
	Blocks& blocks = area.getBlocks();
	Actors& actors = area.getActors();
	Items& items = area.getItems();
	SmallSet<ActorIndex> actorsWhichCannotReserveRotatedPosition;
	SmallSet<Project*> projectsWhichCannotReserveRotatedPosition;
	// Reinstance actors and items.
	for(auto& [onDeck, data] : m_actorsOrItems)
	{
		if(onDeck.isActor())
		{
			const ActorIndex& actor = onDeck.getActor();
			// Update location.
			const BlockIndex location = blocks.translatePosition(data.location, previousPivot, newPivot, previousFacing, newFacing);
			const Facing4 facing = util::rotateFacingByDifference(data.facing, previousFacing, newFacing);
			actors.location_set(actor, location, facing);
			if(actors.mount_isPilot(actor))
				continue;
			// Update path.
			auto& path = actors.move_getPath(actor);
			if(!path.empty())
			{
				uint i = 0;
				for(const BlockIndex& block : path)
				{
					path[i] = blocks.translatePosition(block, previousPivot, newPivot, previousFacing, newFacing);
					++i;
				}
			}
			// Update destination.
			const BlockIndex& destination = actors.move_getDestination(actor);
			if(destination.exists())
			{
				const BlockIndex newDestination = blocks.translatePosition(destination, previousPivot, newPivot, previousFacing, newFacing);
				actors.move_updateDestination(actor, newDestination);
			}
			// Update reservations.
			bool reservationsSuccessful = actors.canReserve_translateAndReservePositions(actor, std::move(data.reservedBlocksAndCallbacks), previousPivot, newPivot, previousFacing, newFacing);
			if(!reservationsSuccessful)
				actorsWhichCannotReserveRotatedPosition.insert(actor);
		}
		else
		{
			// Item.
			const ItemIndex& item = onDeck.getItem();
			const BlockIndex location = blocks.translatePosition(data.location, previousPivot, newPivot, previousFacing, newFacing);
			const Facing4 facing = util::rotateFacingByDifference(data.facing, previousFacing, newFacing);
			items.location_set(item, location, facing);
		}
	}
	// Reinstance projects.
	for(auto& [project, data] : m_projects)
	{
		bool reservationsSuccessful = project->getCanReserve().translateAndReservePositions(blocks, std::move(data.reservedBlocksAndCallbacks), previousPivot, newPivot, previousFacing, newFacing);
		if(!reservationsSuccessful)
			projectsWhichCannotReserveRotatedPosition.insert(project);
		project->setLocation(data.location);
	}
	// Reinstance fluids.
	for(const auto& [block, fluidData] : m_fluids)
	{
		const BlockIndex location = blocks.translatePosition(block, previousPivot, newPivot, previousFacing, newFacing);
		for(const auto& [fluidType, volume] : fluidData)
			blocks.fluid_add(location, volume, fluidType);
	}
	// If an actor cannot reserve the rotated positions they must reset their objective.
	for(const ActorIndex& actor : actorsWhichCannotReserveRotatedPosition)
		actors.objective_canNotCompleteSubobjective(actor);
	// If a project cannot reserve the rotated positions it must reset if it can or otherwise cancel.
	// Resetable projects are typically those created directly by the player, so they should not be cancled.
	for(Project* project : projectsWhichCannotReserveRotatedPosition)
		project->resetOrCancel();
}