#include "draw.h"
#include "sprite.h"
#include "window.h"
#include "../engine/blocks/blocks.h"
#include "../engine/actors/actors.h"
#include "../engine/items/items.h"
#include "../engine/plants.h"
#include "../engine/geometry/cuboid.h"
#include "displayData.h"
#include "interpolate.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
void Draw::view()
{
	Blocks& blocks = m_window.m_area->getBlocks();
	Actors& actors = m_window.m_area->getActors();
	Items& items = m_window.m_area->getItems();
	Plants& plants = m_window.m_area->getPlants();
	m_window.m_gameOverlay.drawTime();
	//m_gameOverlay.drawWeather();
	// Aquire Area read mutex.
	std::lock_guard lock(m_window.m_simulation->m_uiReadMutex);
	// Render block floors, collect actors into single and multi tile groups.
	SmallSet<BlockIndex> singleTileActorBlocks;
	SmallSet<ActorIndex> multiTileActors;
	const Cuboid cuboid = blocks.getZLevel(m_window.m_z);
	const auto zLevelBlocks = cuboid.getView(blocks);
	for(const BlockIndex& block : zLevelBlocks)
	{
		blockFloor(block);
		for(const ActorIndex& actor : blocks.actor_getAll(block))
		{
			if(Shape::getIsMultiTile(actors.getShape(actor)))
				multiTileActors.maybeInsert(actor);
			else
				singleTileActorBlocks.insert(block);
		}
	}
	// Renger Block floors.
	for(const BlockIndex& block : zLevelBlocks)
		blockFloor(block);
	// Render block wall corners.
	for(const BlockIndex& block : zLevelBlocks)
		blockWallCorners(block);
	// Render block walls.
	for(const BlockIndex& block : zLevelBlocks)
		blockWalls(block);
	// Render block features and fluids.
	for(const BlockIndex& block : zLevelBlocks)
		blockFeaturesAndFluids(block);
	// Render block plants.
	for(const BlockIndex& block : zLevelBlocks)
		nonGroundCoverPlant(block);
	// Render items.
	for(const BlockIndex& block : zLevelBlocks)
		item(block);
	// Render block wall tops.
	for(const BlockIndex& block : zLevelBlocks)
		blockWallTops(block);
	// Render Actors.
	// Multi tile actors first.
	//TODO: what if multitile actors overlap?
	for(const ActorIndex& actor : multiTileActors)
		multiTileActor(actor);
	// Single tile actors.
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	for(const BlockIndex& block : singleTileActorBlocks)
	{
		assert(!blocks.actor_empty(block));
		// Multiple actors, cycle through them over time.
		// Use real time rather then m_step to continue cycling while paused.
		auto& actorsInBlock = blocks.actor_getAll(block);
		uint32_t count = actorsInBlock.size();
		if(count == 1)
			singleTileActor(actorsInBlock.front());
		else
		{
			uint8_t index = (seconds % count);
			//TODO: hide some actors from player?
			singleTileActor(actorsInBlock[index]);
		}
	}
	// Designated and project progress.
	if(m_window.m_faction.exists())
	{
		if(m_window.m_area->m_blockDesignations.contains(m_window.m_faction))
		{
			for(const BlockIndex& block : zLevelBlocks)
				maybeDesignated(block);
		}
		for(const BlockIndex& block : zLevelBlocks)
		{
			craftLocation(block);
			Percent projectProgress = blocks.project_getPercentComplete(block, m_window.m_faction);
			if(projectProgress != 0)
				progressBarOnBlock(block, projectProgress);
		}
	}
	// Render item overlays.
	for(const BlockIndex& block : zLevelBlocks)
		itemOverlay(block);
	// Render actor overlays.
	for(const BlockIndex& block : singleTileActorBlocks)
		actorOverlay(blocks.actor_getAll(block).front());

	// Selected.
	if(!m_window.m_selectedBlocks.empty())
	{
		for(const Cuboid& cuboid : m_window.m_selectedBlocks.getCuboids())
			selected(cuboid);
	}
	else if(!m_window.m_selectedActors.empty())
	{
		for(const ActorIndex& actor: m_window.m_selectedActors)
			for(const BlockIndex& block : actors.getBlocks(actor))
				if(blocks.getZ(block) == m_window.m_z)
					selected(block);
	}
	else if(!m_window.m_selectedItems.empty())
	{
		for(const ItemIndex& item: m_window.m_selectedItems)
			for(const BlockIndex& block : items.getBlocks(item))
				if(blocks.getZ(block) == m_window.m_z)
					selected(block);
	}
	else if(!m_window.m_selectedPlants.empty())
		for(const PlantIndex& plant: m_window.m_selectedPlants)
			for(const BlockIndex& block : plants.getBlocks(plant))
				if(blocks.getZ(block) == m_window.m_z)
					selected(block);
	// Selection Box.
	if(m_window.m_firstCornerOfSelection.exists() && sf::Mouse::isButtonPressed(displayData::selectMouseButton))
	{
		auto end = sf::Mouse::getPosition();
		auto start = m_window.m_positionWhereMouseDragBegan;
		uint32_t xSize = std::abs((int32_t)start.x - (int32_t)end.x);
		uint32_t ySize = std::abs((int32_t)start.y - (int32_t)end.y);
		int32_t left = std::min(start.x, end.x);
		int32_t top = std::min(start.y, end.y);
		sf::Vector2f worldPos = m_window.m_window.mapPixelToCoords({left, top});
		sf::RectangleShape square(sf::Vector2f(xSize, ySize));
		square.setFillColor(sf::Color::Transparent);
		square.setOutlineColor(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) ? displayData::cancelColor : displayData::selectColor);
		square.setOutlineThickness(3.f);
		square.setPosition(worldPos);
		m_window.m_window.draw(square);
	}
	// Install or move item.
	if(m_window.m_gameOverlay.m_itemBeingInstalled.exists() || m_window.m_gameOverlay.m_itemBeingMoved.exists())
	{
		assert(m_window.m_blockUnderCursor.exists());
		const BlockIndex& hoverBlock = m_window.m_blockUnderCursor;
		const ItemIndex& item = (m_window.m_gameOverlay.m_itemBeingInstalled.exists() ?
			m_window.m_gameOverlay.m_itemBeingInstalled :
			m_window.m_gameOverlay.m_itemBeingMoved
		);
		auto occupiedBlocks = items.getBlocksWhichWouldBeOccupiedAtLocationAndFacing(item, hoverBlock, m_window.m_gameOverlay.m_facing);
		bool valid = blocks.shape_shapeAndMoveTypeCanEnterEverWithFacing(hoverBlock, items.getShape(item), items.getMoveType(item), m_window.m_gameOverlay.m_facing);
		for(const BlockIndex& block : occupiedBlocks)
			if(!valid)
				invalidOnBlock(block);
			else
				validOnBlock(block);
	}
	// Area Border.
	sf::RectangleShape areaBorder(sf::Vector2f((blocks.m_sizeX.get() * m_window.m_scale), (blocks.m_sizeX.get() * m_window.m_scale) ));
	areaBorder.setOutlineColor(sf::Color::White);
	areaBorder.setFillColor(sf::Color::Transparent);
	areaBorder.setOutlineThickness(3.f);
	areaBorder.setPosition(sf::Vector2f(0,0));
	m_window.m_window.draw(areaBorder);
	// Update Info popup.
	//if(m_gameOverlay.infoPopupIsVisible())
		//m_gameOverlay.updateInfoPopup();
}
// BlockIndex.
void Draw::blockFloor(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	if(blocks.solid_is(block))
	{
		for(const BlockIndex& adjacent : blocks.getDirectlyAdjacent(block))
			if(m_window.m_editMode || blocks.isVisible(adjacent))
			{
				colorOnBlock(block, displayData::materialColors[blocks.solid_get(block)]);
				break;
			}
	}
	else if(m_window.m_editMode || blocks.isVisible(block))
	{
		const BlockIndex& below = blocks.getBlockBelow(block);
		if(below.exists() && blocks.solid_is(below))
		{
			// Draw floor.
			if(blocks.plant_exists(block))
			{
				Plants &plants = m_window.m_area->getPlants();
				const PlantSpeciesId& species = plants.getSpecies(blocks.plant_get(block));
				const PlantSpeciesDisplayData& display = displayData::plantData[species];
				if(display.groundCover)
				{
					//TODO: color.
					imageOnBlock(block, display.image);
					if(m_window.getSelectedPlants().contains(blocks.plant_get(block)))
						outlineOnBlock(block, displayData::selectColor);
					return;
				}
			}
			// No ground cover plant.
			const sf::Color* color = &displayData::materialColors[blocks.solid_get(blocks.getBlockBelow(block))];
			if(blocks.isConstructed(below))
			{
				static sf::Sprite sprite = sprites::make("blockFloor").first;
				spriteOnBlock(block, sprite, color);
			}
			else
			{
				static sf::Sprite sprite = sprites::make("roughFloor").first;
				spriteOnBlock(block, sprite, color);
			}
			// Draw stockpiles.
			if(blocks.stockpile_contains(block, m_window.getFaction()))
				colorOnBlock(block, displayData::stockPileColor);
			// Draw farm fields.
			if(blocks.farm_contains(block, m_window.getFaction()))
				colorOnBlock(block, displayData::farmFieldColor);
			// Draw cliff edge, ramp, or stairs below floor.
			blockWallsFromNextLevelDown(block);
		}
		else if(below.exists() && blocks.fluid_getTotalVolume(below) > Config::maxBlockVolume * displayData::minimumFluidVolumeToSeeFromAboveLevelRatio)
		{
			const FluidTypeId& fluidType = blocks.fluid_getTypeWithMostVolume(below);
			const sf::Color color = displayData::fluidColors[fluidType];
			static sf::Sprite sprite = sprites::make("fluidSurface").first;
			// TODO: Give the shore line some feeling of depth somehow.
			spriteOnBlock(block, sprite, &color);
		}
	}
}
void Draw::blockWallCorners(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	const BlockIndex& west = blocks.getBlockWest(block);
	const BlockIndex& south = blocks.getBlockSouth(block);
	const BlockIndex& below = blocks.getBlockBelow(block);
	if(
		(m_window.m_editMode || blocks.isVisible(block)) &&
		blocks.solid_is(block) && west.exists() && !blocks.solid_is(west) && south.exists() && !blocks.solid_is(south)
	)
	{
		const sf::Color color = displayData::materialColors[blocks.solid_get(block)];
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		auto pair = sprites::make(blocks.isConstructed(block) ? "blockWall" : "roughWall");
		auto sprite = pair.first;
		const Point3D coordinates = blocks.getCoordinates(block);
		sprite.setPosition(((float)coordinates.x.get() - 0.21f) * (float)m_window.m_scale, ((float)coordinates.y.get() + 0.48f) * (float)m_window.m_scale);
		sprite.setScale(scaleRatio, scaleRatio);
		sprite.setColor(color);
		sprite.setRotation(45);
		m_window.getRenderWindow().draw(sprite);
	}
	else if(below.exists())
	{
		const BlockIndex& belowWest = blocks.getBlockWest(below);
		const BlockIndex& belowSouth = blocks.getBlockSouth(below);
		if(
			(m_window.m_editMode || blocks.isVisible(below)) &&
			!blocks.solid_is(block) && blocks.solid_is(below) && belowWest.exists() && !blocks.solid_is(belowWest) && belowSouth.exists() && !blocks.solid_is(belowSouth)
		)
		{
			const sf::Color color = displayData::materialColors[blocks.solid_get(below)];
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(blocks.isConstructed(block) ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			const Point3D coordinates = blocks.getCoordinates(block);
			sprite.setPosition(((float)coordinates.x.get() - 0.21f) * (float)m_window.m_scale, ((float)coordinates.y.get() + 0.48f) * (float)m_window.m_scale);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			sprite.setRotation(45);
			m_window.getRenderWindow().draw(sprite);
		}
	}
}
void Draw::blockWalls(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	if(blocks.solid_is(block))
	{
		auto adjacentPredicate = [&](const BlockIndex& adjacent){
			return adjacent.exists() && (m_window.m_editMode || blocks.isVisible(adjacent)) && !blocks.solid_is(adjacent) && !blocks.blockFeature_contains(adjacent, BlockFeatureType::stairs) && !blocks.blockFeature_contains(adjacent, BlockFeatureType::ramp);
		};
		const Point3D coordinates = blocks.getCoordinates(block);
		const BlockIndex& south = blocks.getBlockSouth(block);
		if(adjacentPredicate(south))
		{
			const sf::Color color = displayData::materialColors[blocks.solid_get(block)];
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(blocks.isConstructed(block) ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition((float)coordinates.x.get() * m_window.m_scale, (float)(coordinates.y.get() + 1) * m_window.m_scale);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		const BlockIndex &west = blocks.getBlockWest(block);
		if(adjacentPredicate(west))
		{
			const sf::Color color = displayData::materialColors[blocks.solid_get(block)];
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(blocks.isConstructed(block) ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition((float)coordinates.x.get() * m_window.m_scale, (float)coordinates.y.get() * m_window.m_scale);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			sprite.setRotation(90);
			m_window.getRenderWindow().draw(sprite);
		}
	}
}
void Draw::blockWallTops(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	if(blocks.solid_is(block))
	{
		auto adjacentPredicate = [&](const BlockIndex& adjacent){
			return adjacent.exists() && (m_window.m_editMode || blocks.isVisible(adjacent)) && !blocks.solid_is(adjacent) && !blocks.blockFeature_contains(adjacent, BlockFeatureType::stairs) && !blocks.blockFeature_contains(adjacent, BlockFeatureType::ramp);
		};
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		const sf::Color color = displayData::materialColors[blocks.solid_get(block)];
		float offset = displayData::wallTopOffsetRatio * m_window.m_scale;
		const Point3D coordinates = blocks.getCoordinates(block);
		const BlockIndex& north = blocks.getBlockNorth(block);
		if(adjacentPredicate(north))
		{
			auto pair = sprites::make(blocks.isConstructed(block) ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setRotation(180);
			sprite.setPosition((float)(coordinates.x.get() + 1) * m_window.m_scale, ((float)coordinates.y.get() * m_window.m_scale) + offset);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		const BlockIndex& east = blocks.getBlockEast(block);
		if(adjacentPredicate(east))
		{
			auto pair = sprites::make(blocks.isConstructed(block) ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setRotation(270);
			sprite.setPosition((((float)coordinates.x.get() + 1) * m_window.m_scale) - offset, ((float)coordinates.y.get() + 1) * m_window.m_scale);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		const BlockIndex& west = blocks.getBlockWest(block);
		if(adjacentPredicate(west))
		{
			auto pair = sprites::make(blocks.isConstructed(block) ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setRotation(90);
			sprite.setPosition(((float)coordinates.x.get() * m_window.m_scale) + offset, ((float)coordinates.y.get() * m_window.m_scale));
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		const BlockIndex& south = blocks.getBlockSouth(block);
		if(adjacentPredicate(south))
		{
			auto pair = sprites::make(blocks.isConstructed(block) ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setPosition(((float)coordinates.x.get() * m_window.m_scale), ((float)(coordinates.y.get() + 1) * m_window.m_scale) - offset);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
	}
}
void Draw::blockFeaturesAndFluids(const BlockIndex& block)
{
	// BlockIndex Features
	//TODO: Draw order.
	Blocks& blocks = m_window.m_area->getBlocks();
	if(!blocks.blockFeature_empty(block))
	{
		const BlockIndex& north = blocks.getBlockNorth(block);
		const BlockIndex& south = blocks.getBlockSouth(block);
		for(const BlockFeature& blockFeature : blocks.blockFeature_getAll(block))
		{
			sf::Color* color = &displayData::materialColors[blockFeature.materialType];
			if(blockFeature.blockFeatureType == &BlockFeatureType::hatch)
			{
				static sf::Sprite hatch = getCenteredSprite("hatch");
				spriteOnBlockCentered(block, hatch, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::floorGrate)
			{
				static sf::Sprite floorGrate = getCenteredSprite("floorGrate");
				spriteOnBlockCentered(block, floorGrate, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::stairs)
			{
				static sf::Sprite stairs = getCenteredSprite("stairs");
				Facing4 facing = rampOrStairsFacing(block);
				stairs.setRotation((uint)facing * 90);
				stairs.setOrigin(16,19);
				spriteOnBlockCentered(block, stairs, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::ramp)
			{
				static sf::Sprite ramp = getCenteredSprite("ramp");
				Facing4 facing = rampOrStairsFacing(block);
				ramp.setRotation((uint)facing * 90);
				ramp.setOrigin(16,19);
				spriteOnBlockCentered(block, ramp, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::floodGate)
			{
				static sf::Sprite floodGate = getCenteredSprite("floodGate");
				// Default floodGate image leads north-south, maybe rotate.
				if(north.empty() || blocks.solid_is(north) || south.empty() || blocks.solid_is(south))
					floodGate.setRotation(90);
				else
					floodGate.setRotation(0);
				spriteOnBlockCentered(block, floodGate, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::fortification)
			{
				static sf::Sprite fortification = getCenteredSprite("fortification");
				// Default fortification image leads north-south, maybe rotate.
				if(north.empty() || blocks.solid_is(north) || !blocks.shape_canStandIn(north) || south.empty() || blocks.solid_is(south) || !blocks.shape_canStandIn(south))
					fortification.setRotation(90);
				else
					fortification.setRotation(0);
				spriteOnBlockCentered(block, fortification, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::door)
			{
				static sf::Sprite door = getCenteredSprite("door");
				// Default door image leads north-south, maybe rotate.
				if(north.empty() || blocks.solid_is(north) || !blocks.shape_canStandIn(north) || south.empty() || blocks.solid_is(south) || !blocks.shape_canStandIn(south))
					door.setRotation(90);
				else
					door.setRotation(0);
				spriteOnBlockCentered(block, door, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::flap)
			{
				static sf::Sprite flap = getCenteredSprite("flap");
				// Default flap image leads north-south, maybe rotate.
				if(north.empty() || blocks.solid_is(north) || !blocks.shape_canStandIn(north) || south.empty() || blocks.solid_is(south) || !blocks.shape_canStandIn(south))
					flap.setRotation(90);
				else
					flap.setRotation(0);
				spriteOnBlockCentered(block, flap, color);
			}
		}
	}
	else
	{
		const BlockIndex& below = blocks.getBlockBelow(block);
		if(below.exists() && !blocks.solid_is(below))
		{
			// Show tops of stairs and ramps from next level down.
			if(blocks.blockFeature_contains(below, BlockFeatureType::stairs))
			{
				const BlockFeature& blockFeature = *blocks.blockFeature_at(below, BlockFeatureType::stairs);
				sf::Color* color = &displayData::materialColors[blockFeature.materialType];
				static sf::Sprite stairs = getCenteredSprite("stairs");
				Facing4 facing = rampOrStairsFacing(below);
				stairs.setRotation((uint)facing * 90);
				stairs.setOrigin(16,19);
				stairs.setTextureRect({0,0,32,16});
				spriteOnBlockCentered(block, stairs, color);
			}
			else if(blocks.blockFeature_contains(below, BlockFeatureType::ramp))
			{
				const BlockFeature& blockFeature = *blocks.blockFeature_at(below, BlockFeatureType::ramp);
				sf::Color* color = &displayData::materialColors[blockFeature.materialType];
				static sf::Sprite ramp = getCenteredSprite("ramp");
				Facing4 facing = rampOrStairsFacing(below);
				ramp.setRotation((uint)facing * 90);
				ramp.setOrigin(16,19);
				ramp.setTextureRect({0,0,32,16});
				spriteOnBlockCentered(block, ramp, color);
			}
		}
	}
	// Fluids
	if(blocks.fluid_getTotalVolume(block) != 0)
	{
		const FluidTypeId& fluidType = blocks.fluid_getTypeWithMostVolume(block);
		CollisionVolume volume = blocks.fluid_volumeOfTypeContains(block, fluidType);
		const sf::Color color = displayData::fluidColors[fluidType];
		colorOnBlock(block, color);
		stringOnBlock(block, std::to_wstring(volume.get()), sf::Color::Black);
	}
}
void Draw::blockWallsFromNextLevelDown(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	const BlockIndex& below = blocks.getBlockBelow(block);
	if(below.empty() || !blocks.solid_is(below))
		return;
	const BlockIndex& belowSouth = blocks.getBlockSouth(below);
	if(belowSouth.exists() && !blocks.solid_is(belowSouth))
	{
		sf::Sprite* sprite;
		MaterialTypeId materialType;
		if(blocks.isConstructed(block))
		{
			static sf::Sprite blockWall = getCenteredSprite("blockWall");
			sprite = &blockWall;
			materialType = blocks.solid_get(below);
		}
		else
		{
			static sf::Sprite roughWall = getCenteredSprite("roughWall");
			sprite = &roughWall;
			materialType = blocks.solid_get(below);
		}
		sprite->setTextureRect({0, 0, 32, 18});
		assert(materialType.exists());
		assert(displayData::materialColors.contains(materialType));
		sf::Color color = displayData::materialColors[materialType];
		const Point3D belowSouthPosition = blocks.getCoordinates(belowSouth);
		sf::Vector2f position{(((float)belowSouthPosition.x.get() + 0.5f) * (float)m_window.m_scale), (((float)belowSouthPosition.y.get() + 0.5f - displayData::wallOffset) * (float)m_window.m_scale)};
		spriteAt(*sprite, position, &color);
	}
	const BlockIndex& belowWest = blocks.getBlockWest(below);
	if(belowWest.exists() && !blocks.solid_is(belowWest))
	{
		static sf::Sprite* sprite;
		MaterialTypeId materialType;
		if(blocks.isConstructed(block))
		{
			static sf::Sprite blockWall = getCenteredSprite("blockWall");
			sprite = &blockWall;
		}
		else
		{
			static sf::Sprite roughWall = getCenteredSprite("roughWall");
			sprite = &roughWall;
		}
		materialType = blocks.solid_get(below);
		sprite->setTextureRect({0, 0, 32, 18});
		sprite->setRotation(90);
		assert(displayData::materialColors.contains(materialType));
		sf::Color color = displayData::materialColors[materialType];
		const Point3D belowWestPosition = blocks.getCoordinates(belowWest);
		sf::Vector2f position{(((float)belowWestPosition.x.get() + 0.5f + displayData::wallOffset) * (float)m_window.m_scale), (((float)belowWestPosition.y.get() + 0.5f) * (float)m_window.m_scale)};
		spriteAt(*sprite, position, &color);
	}
}
void Draw::validOnBlock(const BlockIndex& block)
{
	colorOnBlock(block, {0, 255, 0, 122});
}
void Draw::invalidOnBlock(const BlockIndex& block)
{
	colorOnBlock(block, {255, 0, 0, 122});
}
void Draw::colorOnBlock(const BlockIndex& block, const sf::Color color)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, m_window.m_scale));
	square.setFillColor(color);
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	square.setPosition((float)coordinates.x.get() * m_window.m_scale, ((float)coordinates.y.get() * m_window.m_scale));
	m_window.getRenderWindow().draw(square);
}
void Draw::maybeDesignated(const BlockIndex& block)
{
	BlockDesignation designation = m_window.m_area->m_blockDesignations.getForFaction(m_window.m_faction).getDisplayDesignation(block);
	if(designation == BlockDesignation::BLOCK_DESIGNATION_MAX)
		return;
	static std::unordered_map<BlockDesignation, sf::Sprite> designationSprites{
		{BlockDesignation::Dig, getCenteredSprite("pick")},
		{BlockDesignation::Construct, getCenteredSprite("mallet")},
		{BlockDesignation::FluidSource, getCenteredSprite("bucket")},
		{BlockDesignation::GivePlantFluid, getCenteredSprite("bucket")},
		{BlockDesignation::SowSeeds, getCenteredSprite("seed")},
		{BlockDesignation::Harvest, getCenteredSprite("hand")},
		{BlockDesignation::Rescue, getCenteredSprite("hand")},
		{BlockDesignation::Sleep, getCenteredSprite("sleep")},
		{BlockDesignation::WoodCutting, getCenteredSprite("axe")},
		{BlockDesignation::StockPileHaulFrom, getCenteredSprite("hand")},
		{BlockDesignation::StockPileHaulTo, getCenteredSprite("open")}
	};
	spriteOnBlockCentered(block, designationSprites.at(designation), &displayData::selectColor);
}
void Draw::craftLocation(const BlockIndex& block)
{
	const CraftStepTypeCategoryId& craftDisplay = m_window.m_area->m_hasCraftingLocationsAndJobs.getForFaction(m_window.getFaction()).getDisplayStepTypeCategoryForLocation(block);
	if(craftDisplay.exists())
	{
		static sf::Sprite craftSpot = getCenteredSprite("tool");
		spriteOnBlockCentered(block, craftSpot, &displayData::selectColor);
	}
}
sf::Sprite Draw::getCenteredSprite(std::string name)
{
	auto pair = sprites::make(name);
	pair.first.setOrigin(pair.second);
	return pair.first;
}
// Origin is assumed to already be set in sprite on block.
void Draw::spriteOnBlockWithScaleCentered(const BlockIndex& block, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = m_window.m_scale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	sprite.setPosition(((float)coordinates.x.get() + 0.5f) * windowScale, ((float)coordinates.y.get() + 0.5f) * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteOnBlockWithScale(const BlockIndex& block, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = m_window.m_scale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	sprite.setPosition((float)coordinates.x.get() * windowScale, (float)coordinates.y.get() * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color)
{
	spriteAtWithScale(sprite, position, 1, color);
}
void Draw::spriteAtWithScale(sf::Sprite& sprite, sf::Vector2f position, float scale, const sf::Color* color)
{
	sprite.setPosition(position);
	float scaleRatio = scale * (float)m_window.m_scale / (float)displayData::defaultScale;
	sprite.setScale(scaleRatio, scaleRatio);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteOnBlock(const BlockIndex& block, sf::Sprite& sprite, const sf::Color* color)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	sf::Vector2f position{(float)(coordinates.x.get()* m_window.m_scale), (float)(coordinates.y.get()* m_window.m_scale)};
	spriteAt(sprite, position, color);
}
void Draw::spriteOnBlockCentered(const BlockIndex& block, sf::Sprite& sprite, const sf::Color* color)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	sf::Vector2f position{(((float)coordinates.x.get() + 0.5f) * (float)m_window.m_scale), (((float)coordinates.y.get() + 0.5f) * (float)m_window.m_scale)};
	spriteAt(sprite, position, color);
}
void Draw::imageOnBlock(const BlockIndex& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	spriteOnBlock(block, pair.first, color);
}
// By default images are top aligned.
void Draw::imageOnBlockWestAlign(const BlockIndex& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(270);
	spriteOnBlock(block, pair.first, color);
}
void Draw::imageOnBlockEastAlign(const BlockIndex& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(90);
	spriteOnBlock(block, pair.first, color);
}
void Draw::imageOnBlockSouthAlign(const BlockIndex& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(180);
	spriteOnBlock(block, pair.first, color);
}
void Draw::progressBarOnBlock(const BlockIndex& block, Percent progress)
{
	float scaledUnit = getScaledUnit();
	sf::RectangleShape outline(sf::Vector2f(m_window.m_scale, scaledUnit * (2 + displayData::progressBarThickness)));
	outline.setFillColor(displayData::progressBarOutlineColor);
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	outline.setPosition((float)coordinates.x.get() * m_window.m_scale, (float)coordinates.y.get() * m_window.m_scale);
	m_window.getRenderWindow().draw(outline);
	float progressWidth = util::scaleByPercent(m_window.m_scale, progress) - (scaledUnit * 2);
	sf::RectangleShape rectangle(sf::Vector2f(progressWidth, scaledUnit * displayData::progressBarThickness));
	rectangle.setFillColor(displayData::progressBarColor);
	rectangle.setPosition(((float)coordinates.x.get() * m_window.m_scale) + scaledUnit, ((float)coordinates.y.get() * m_window.m_scale) + scaledUnit);
	m_window.getRenderWindow().draw(rectangle);
}
void Draw::selected(const BlockIndex& block) { outlineOnBlock(block, displayData::selectColor); }
void Draw::selected(const Cuboid& cuboid)
{
	// Check if cuboid intersects with current z level
	if(cuboid.m_lowest.z > m_window.m_z || cuboid.m_highest.z < m_window.m_z)
		return;
	static constexpr uint thickness = 3;
	// Set Dimensions.
	const uint xSize = (cuboid.m_highest.x - cuboid.m_lowest.x).get() + 1;
	const uint ySize = (cuboid.m_highest.y - cuboid.m_lowest.y).get() + 1;
	sf::RectangleShape square(sf::Vector2f(xSize * m_window.m_scale - (thickness*2), ySize * m_window.m_scale - (thickness*2)));
	// Set Color.
	square.setFillColor(sf::Color::Transparent);
	square.setOutlineColor(displayData::selectColor);
	square.setOutlineThickness(thickness);
	// Set Position.
	square.setPosition(((float)cuboid.m_lowest.x.get() * m_window.m_scale) + thickness, ((float)cuboid.m_lowest.y.get() * m_window.m_scale) + thickness);
	m_window.getRenderWindow().draw(square);
}
void Draw::outlineOnBlock(const BlockIndex& block, const sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale - (thickness*2), m_window.m_scale - (thickness*2)));
	square.setFillColor(sf::Color::Transparent);
	square.setOutlineColor(color);
	square.setOutlineThickness(thickness);
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	square.setPosition(((float)coordinates.x.get() * m_window.m_scale) + thickness, ((float)coordinates.y.get() * m_window.m_scale) + thickness);
	m_window.getRenderWindow().draw(square);
}
void Draw::stringOnBlock(const BlockIndex& block, std::wstring string, const sf::Color color, float offsetX, float offsetY )
{
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	return stringAtPosition(string, {
		((float)coordinates.x.get() + offsetX) * m_window.m_scale,
		((float)coordinates.y.get() + offsetY) * m_window.m_scale
	}, color);

}
void Draw::stringAtPosition(const std::wstring string, const sf::Vector2f position, const sf::Color color, float offsetX, float offsetY )
{
	// Don't draw text which would be too small to read comfortably.
	if(m_window.m_scale < displayData::defaultScale)
		return;
	sf::Text text;
	text.setFont(m_window.m_font);
	text.setFillColor(color);
	text.setCharacterSize(m_window.m_scale * displayData::ratioOfScaleToFontSize);
	text.setString(string);
	text.setPosition( position.x + offsetX, position.y + offsetY);
	auto center = text.getGlobalBounds().width / 2;
	auto xOrigin = std::round(center);
	text.setOrigin(xOrigin, 0);
	m_window.getRenderWindow().draw(text);
}
// Plant.
void Draw::nonGroundCoverPlant(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	Plants& plants = m_window.m_area->getPlants();
	if(!blocks.plant_exists(block))
		return;
	const PlantIndex& plant = blocks.plant_get(block);
	PlantSpeciesDisplayData& display = displayData::plantData[plants.getSpecies(plant)];
	// Ground cover plants are drawn with floors.
	if(display.groundCover)
		return;
	auto& occupiedBlocks = plants.getBlocks(plant);
	if(PlantSpecies::getIsTree(plants.getSpecies(plant)) && occupiedBlocks.size() != 1)
	{
		if(block == plants.getLocation(plant) && occupiedBlocks.size() > 2)
		{
			static sf::Sprite trunk = getCenteredSprite("trunk");
			spriteOnBlockCentered(block, trunk);
		}
		else
		{
			const Point3D coordinates = blocks.getCoordinates(block);
			const BlockIndex& plantLocation = plants.getLocation(plant);
			const Point3D plantLocationCoordinates = blocks.getCoordinates(plantLocation);
			if(coordinates.x == plantLocationCoordinates.x && coordinates.y == plantLocationCoordinates.y)
			{
				const BlockIndex& above = blocks.getBlockAbove(block);
				if(above.exists() && blocks.plant_exists(above) && blocks.plant_get(above) == plant)
				{
					static sf::Sprite trunk = getCenteredSprite("trunkWithBranches");
					spriteOnBlockCentered(block, trunk);
					static sf::Sprite trunkLeaves = getCenteredSprite("trunkLeaves");
					spriteOnBlockCentered(block, trunkLeaves);
				}
				else
				{
					static sf::Sprite treeTop = getCenteredSprite(display.image);
					spriteOnBlockCentered(block, treeTop);
				}
			}
			else
			{
				float angle = 45.f * (uint)blocks.facingToSetWhenEnteringFromIncludingDiagonal(block, plantLocation);
				static sf::Sprite branch = getCenteredSprite("branch");
				branch.setRotation(angle);
				spriteOnBlockCentered(block, branch);
				static sf::Sprite branchLeaves = getCenteredSprite("branchLeaves");
				branchLeaves.setRotation(angle);
				spriteOnBlockCentered(block, branchLeaves);
			}
		}
	}
	else
	{
		auto pair = sprites::make(display.image);
		//TODO: color
		pair.first.setOrigin(pair.second);
		float scale = plants.getBlocks(plant).size() == 1 ? ((float)plants.getPercentGrown(plant).get() / 100.f) : 1;
		spriteOnBlockWithScaleCentered(block, pair.first, scale);
		if(m_window.getSelectedPlants().contains(plant))
			outlineOnBlock(block, displayData::selectColor);
	}
}
// Item.
void Draw::item(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	if(blocks.item_empty(block))
		return;
	const ItemIndex& itemToDraw = blocks.item_getAll(block).front();
	item(itemToDraw, blockToPositionCentered(block));
}
void Draw::item(const ItemIndex& item, sf::Vector2f position)
{
	Items& items = m_window.m_area->getItems();
	ItemTypeDisplayData& display = displayData::itemData[items.getItemType(item)];
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	const sf::Color materialColor = displayData::materialColors[items.getMaterialType(item)];
	sprite.setColor(materialColor);
	spriteAtWithScale(sprite, position, display.scale);
}
void Draw::itemOverlay(const ItemIndex& item, sf::Vector2f position)
{
	Items& items = m_window.m_area->getItems();
	Quantity quantity = items.getQuantity(item);
	if(quantity != 1)
		stringAtPosition(std::to_wstring(quantity.get()), position, sf::Color::White);
}
void Draw::actorOverlay(const ActorIndex& actor)
{
	Actors& actors = m_window.m_area->getActors();
	sf::Vector2f location = blockToPosition(actors.getLocation(actor));
	location.y -= (float)m_window.m_scale / 3.5f;
	if(!actors.sleep_isAwake(actor))
	{
		sf::Vector2f sleepTextLocation = location;
		sleepTextLocation.y += m_window.m_scale / 3.5f;
		stringAtPosition(L"zzz", sleepTextLocation, sf::Color::Cyan);
	}
}
void Draw::itemOverlay(const BlockIndex& block)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	Items& items = m_window.m_area->getItems();
	if(blocks.item_empty(block))
		return;
	const ItemIndex& item = blocks.item_getAll(block).front();
	if(blocks.item_getAll(block).size() > 1)
		stringOnBlock(block, std::to_wstring(blocks.item_getAll(block).size()), sf::Color::Magenta);
	else if(items.getQuantity(item) != 1)
		stringOnBlock(block, std::to_wstring(items.getQuantity(item).get()), sf::Color::White);
	if(m_window.getSelectedItems().contains(item))
		outlineOnBlock(block, displayData::selectColor);
	if(m_window.getFaction().exists() && !items.stockpile_canBeStockPiled(item, m_window.getFaction()))
		inaccessableSymbol(block);
}
// Actor.
void Draw::singleTileActor(const ActorIndex& actor)
{
	Actors& actors = m_window.m_area->getActors();
	const BlockIndex& block = actors.getLocation(actor);
	AnimalSpeciesDisplayData& display = displayData::actorData[actors.getSpecies(actor)];
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	spriteOnBlockWithScaleCentered(block, sprite, ((float)actors.getPercentGrown(actor).get() + 10.f) / 110.f);
	if(actors.canPickUp_exists(actor))
	{
		const ActorOrItemIndex& isCarrying = actors.canPickUp_getPolymorphic(actor);
		if(isCarrying.isItem())
		{
			sf::Vector2f position = blockToPositionCentered(block);
			position.y += getScaledUnit() * 4;
			const ItemIndex& itemToDraw = isCarrying.getItem();
			item(itemToDraw, position);
		}
		else
		{
			assert(isCarrying.isActor());
			// TODO: Draw actors being carried.
		}
	}
	if(m_window.getSelectedActors().contains(actor))
		outlineOnBlock(block, displayData::selectColor);
}
void Draw::multiTileActor(const ActorIndex& actor)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	Actors& actors = m_window.m_area->getActors();
	AnimalSpeciesDisplayData& display = displayData::actorData[actors.getSpecies(actor)];
	auto [sprite, origin] = sprites::make(display.image);
	const OccupiedBlocksForHasShape& occupiedBlocks = actors.getBlocks(actor);
	// TODO: move display scale to display data.
	if(Shape::getDisplayScale(actors.getShape(actor)) == 1)
	{
		const Point3D& actorLocationCoordinates = blocks.getCoordinates(actors.getLocation(actor));
		const BlockIndex& location = m_window.m_area->getBlocks().getIndex(actorLocationCoordinates.x, actorLocationCoordinates.y, m_window.m_z);
		spriteOnBlock(location, sprite, &display.color);
		multiTileBorder(occupiedBlocks, displayData::actorOutlineColor, 1);
	}
	else
	{
		BlockIndex topLeft;
		for(const BlockIndex& block : occupiedBlocks)
		{
			const Point3D blockCoordinates = blocks.getCoordinates(block);
			if(blockCoordinates.z == m_window.m_z)
			{
				if(topLeft.empty())
					topLeft = block;
				else
				{
					const Point3D topLeftCoordinates = blocks.getCoordinates(topLeft);
					if(blockCoordinates.x < topLeftCoordinates.x || blockCoordinates.y < topLeftCoordinates.y)
						topLeft = block;
				}
			}
		}
		spriteOnBlockWithScale(topLeft, sprite, Shape::getDisplayScale(actors.getShape(actor)), &display.color);
	}
}
void Draw::multiTileBorder(const OccupiedBlocksForHasShape& blocksOccpuied, sf::Color color, float thickness)
{
	Blocks& blocks = m_window.m_area->getBlocks();
	for(const BlockIndex& block : blocksOccpuied)
	{
		for(BlockIndex& adjacent : blocks.getAdjacentOnSameZLevelOnly(block))
			if(!blocksOccpuied.contains(adjacent))
			{
				Facing4 facing = blocks.facingToSetWhenEnteringFrom(adjacent, block);
				borderSegmentOnBlock(block, facing, color, thickness);
			}
	}
}
void Draw::borderSegmentOnBlock(const BlockIndex& block, const Facing4& facing, sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, thickness));
	square.setFillColor(color);
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	switch((uint)facing)
	{
		case 0:
			// do nothing
			square.setPosition(
				(float)coordinates.x.get() * m_window.m_scale,
				(float)coordinates.y.get() * m_window.m_scale
			);
			break;
		case 1:
			square.setRotation(90);
			square.setPosition(
				(((float)coordinates.x.get() + 1.f) * m_window.m_scale) - thickness,
				(float)(coordinates.y.get() * m_window.m_scale)
			);
		break;
		case 2:
			square.setPosition(
				(float)coordinates.x.get() * m_window.m_scale,
				(((float)coordinates.y.get() + 1) * m_window.m_scale) - thickness
			);
			break;

		case 3:
			square.setRotation(90);
			square.setPosition(
				(float)coordinates.x.get() * m_window.m_scale,
				(float)coordinates.y.get() * m_window.m_scale
			);
			break;
	}
	m_window.getRenderWindow().draw(square);
}
Facing4 Draw::rampOrStairsFacing(const BlockIndex& block) const
{
	Blocks& blocks = m_window.m_area->getBlocks();
	static auto canConnectToAbove = [&](const BlockIndex& block) -> bool{
		const BlockIndex& above = blocks.getBlockAbove(block);
		return above.exists() && !blocks.blockFeature_contains(block, BlockFeatureType::stairs) && !blocks.blockFeature_contains(block, BlockFeatureType::ramp) && blocks.shape_canStandIn(above);
	};
	const BlockIndex& above = blocks.getBlockAbove(block);
	if(above.empty())
		return Facing4::North;
	Facing4 backup = Facing4::North;
	const BlockIndex& north = blocks.getBlockNorth(block);
	const BlockIndex& south = blocks.getBlockSouth(block);
	if(north.exists() && canConnectToAbove(north))
	{
		if(!blocks.solid_is(south) && blocks.shape_canStandIn(south) &&
			!blocks.blockFeature_contains(south, BlockFeatureType::stairs) &&
			!blocks.blockFeature_contains(south, BlockFeatureType::stairs)
		)
			return Facing4::North;
	}
	const BlockIndex& east = blocks.getBlockEast(block);
	const BlockIndex& west = blocks.getBlockWest(block);
	if(east.exists() && canConnectToAbove(east))
	{
		if(!blocks.solid_is(west) && blocks.shape_canStandIn(west))
			return Facing4::East;
		else
			backup = Facing4::East;
	}
	if(south.exists() && canConnectToAbove(south))
	{
		if(!blocks.solid_is(north) && blocks.shape_canStandIn(north) &&
			!blocks.blockFeature_contains(north, BlockFeatureType::stairs) &&
			!blocks.blockFeature_contains(north, BlockFeatureType::stairs)
		)
			return Facing4::South;
		else
			backup = Facing4::South;
	}
	if(west.exists() && canConnectToAbove(west))
	{
		if(!blocks.solid_is(east) && blocks.shape_canStandIn(block))
			return Facing4::West;
		else
			backup = Facing4::West;
	}
	return backup;
}
sf::Vector2f Draw::blockToPosition(const BlockIndex& block) const
{
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	return {((float)coordinates.x.get() * m_window.m_scale), ((float)coordinates.y.get() * m_window.m_scale)};
}
sf::Vector2f Draw::blockToPositionCentered(const BlockIndex& block) const
{
	Blocks& blocks = m_window.m_area->getBlocks();
	const Point3D coordinates = blocks.getCoordinates(block);
	return {((float)coordinates.x.get() + 0.5f) * m_window.m_scale, ((float)coordinates.y.get() + 0.5f) * m_window.m_scale};
}
void Draw::accessableSymbol(const BlockIndex& block)
{
	stringOnBlock(block, L"o", sf::Color::Green, 0.1, 0.6);
}
void Draw::inaccessableSymbol(const BlockIndex& block)
{
	stringOnBlock(block, L"x", sf::Color::Red, 0.1, 0.6);
}
float Draw::getScaledUnit() const { return float(m_window.m_scale) / float(displayData::defaultScale); }
