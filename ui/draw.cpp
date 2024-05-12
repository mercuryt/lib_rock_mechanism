#include "draw.h"
#include "sprite.h"
#include "window.h"
#include "../engine/block.h"
#include "../engine/actor.h"
#include "../engine/item.h"
#include "../engine/plant.h"
#include "displayData.h"
#include "interpolate.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
void Draw::view()
{
	m_window.m_gameOverlay.drawTime();
	//m_gameOverlay.drawWeather();
	// Aquire Area read mutex.
	std::lock_guard lock(m_window.m_simulation->m_uiReadMutex);
	// Render block floors, collect actors into single and multi tile groups.
	std::unordered_set<const Block*> singleTileActorBlocks;
	std::unordered_set<const Actor*> multiTileActors;
	for(Block& block : m_window.m_area->getZLevel(m_window.m_z))
	{
		blockFloor(block);
		for(const Actor* actor : block.m_hasActors.getAllConst())
		{
			if(actor->m_shape->isMultiTile)
				multiTileActors.insert(actor);
			else
				singleTileActorBlocks.insert(&block);
		}
	}
	// Render block wall corners.
	auto zLevelBlocks = m_window.m_area->getZLevel(m_window.m_z);
	for(Block& block : zLevelBlocks)
		blockWallCorners(block);
	// Render block walls.
	for(Block& block : zLevelBlocks)
		blockWalls(block);
	// Render block features and fluids.
	for(Block& block : zLevelBlocks)
		blockFeaturesAndFluids(block);
	// Render block plants.
	for(Block& block : zLevelBlocks)
		nonGroundCoverPlant(block);
	// Render items.
	for(Block& block : zLevelBlocks)
		item(block);
	// Render block wall tops.
	for(Block& block : zLevelBlocks)
		blockWallTops(block);
	// Render Actors.
	// Do multi tile actors first.
	//TODO: what if multitile actors overlap?
	for(const Actor* actor : multiTileActors)
		multiTileActor(*actor);
	// Do single tile actors.
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	for(const Block* block : singleTileActorBlocks)
	{
		assert(!block->m_hasActors.empty());
		// Multiple actors, cycle through them over time.
		// Use real time rather then m_step to continue cycling while paused.
		uint32_t count = block->m_hasActors.getAllConst().size();
		if(count == 1)
			singleTileActor(*block->m_hasActors.getAllConst().front());
		else
		{
			uint8_t index = (seconds % count);
			//TODO: hide some actors from player?
			const std::vector<Actor*> actors = block->m_hasActors.getAllConst();
			singleTileActor(*actors[index]);
		}
	}
	// Designated and project progress.
	if(m_window.m_faction)
		for(Block& block : m_window.m_area->getZLevel(m_window.m_z))
		{
			if(block.m_hasDesignations.containsFaction(*m_window.m_faction))
				designated(block);
			craftLocation(block);
			Percent projectProgress = block.m_hasProjects.getProjectPercentComplete(*m_window.m_faction);
			if(block.m_hasProjects.get(*m_window.m_faction))
				progressBarOnBlock(block, projectProgress);
		}
	// Render item overlays.
	for(Block& block : m_window.m_area->getZLevel(m_window.m_z))
		itemOverlay(block);
	// Render actor overlays.
	for(const Block* block : singleTileActorBlocks)
		actorOverlay(*block->m_hasActors.getAllConst().front());
	
	// Selected.
	if(!m_window.m_selectedBlocks.empty())
	{
		for(Block* block :m_window. m_selectedBlocks)
			if(block->m_z == m_window.m_z)
				selected(*block);
	}
	else if(!m_window.m_selectedActors.empty())
	{
		for(Actor* actor: m_window.m_selectedActors)
			for(Block* block : actor->m_blocks)
				if(block->m_z == m_window.m_z)
					selected(*block);
	}
	else if(!m_window.m_selectedItems.empty())
	{
		for(Item* item: m_window.m_selectedItems)
			for(Block* block : item->m_blocks)
				if(block->m_z == m_window.m_z)
					selected(*block);
	}
	else if(!m_window.m_selectedPlants.empty())
		for(Plant* plant: m_window.m_selectedPlants)
			for(Block* block : plant->m_blocks)
				if(block->m_z == m_window.m_z)
					selected(*block);
	// Selection Box.
	if(m_window.m_firstCornerOfSelection && sf::Mouse::isButtonPressed(displayData::selectMouseButton))
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
		square.setOutlineColor(displayData::selectColor);
		square.setOutlineThickness(3.f);
		square.setPosition(worldPos);
		m_window.m_window.draw(square);
	}
	// Install item.
	if(m_window.m_gameOverlay.m_itemBeingInstalled)
	{
		Block& hoverBlock = m_window.getBlockUnderCursor();
		auto blocks = m_window.m_gameOverlay.m_itemBeingInstalled->getBlocksWhichWouldBeOccupiedAtLocationAndFacing(hoverBlock, m_window.m_gameOverlay.m_facing);
		bool valid = hoverBlock.m_hasShapes.canEnterEverWithFacing(*m_window.m_gameOverlay.m_itemBeingInstalled, m_window.m_gameOverlay.m_facing);
		for(Block* block : blocks)
			if(!valid)
				invalidOnBlock(*block);
			else
				validOnBlock(*block);
	}
	// Area Border.
	sf::RectangleShape areaBorder(sf::Vector2f((m_window.m_scale * m_window.m_area->m_sizeX), (m_window.m_scale * m_window.m_area->m_sizeX) ));
	areaBorder.setOutlineColor(sf::Color::White);
	areaBorder.setFillColor(sf::Color::Transparent);
	areaBorder.setOutlineThickness(3.f);
	areaBorder.setPosition(sf::Vector2f(0,0));
	m_window.m_window.draw(areaBorder);
	// Update Info popup.
	//if(m_gameOverlay.infoPopupIsVisible())
		//m_gameOverlay.updateInfoPopup();
}
// Block.
void Draw::blockFloor(const Block& block)
{
	if(block.isSolid())
	{
		for(Block* adjacent : block.m_adjacents)
			if(adjacent && (m_window.m_editMode || adjacent->m_visible))
			{
				assert(displayData::materialColors.contains(&block.getSolidMaterial()));
				colorOnBlock(block, displayData::materialColors.at(&block.getSolidMaterial()));
				break;
			}
	}
	else if(block.m_visible)
	{
		if(block.getBlockBelow() && block.getBlockBelow()->isSolid())
		{
			// Draw floor.
			if(block.m_hasPlant.exists())
			{
				const PlantSpecies& species = block.m_hasPlant.get().m_plantSpecies;
				const PlantSpeciesDisplayData& display = displayData::plantData.at(&species);
				if(display.groundCover)
				{
					//TODO: color.
					imageOnBlock(block, display.image);
					if(m_window.getSelectedPlants().contains(const_cast<Plant*>(&block.m_hasPlant.get())))
						outlineOnBlock(block, displayData::selectColor);
					return;
				}
			}
			// No ground cover plant.
			const sf::Color* color = &displayData::materialColors.at(&block.getBlockBelow()->getSolidMaterial());
			if(block.getBlockBelow()->isConstructed())
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
			if(block.m_isPartOfStockPiles.contains(*m_window.getFaction()))
				colorOnBlock(block, displayData::stockPileColor);
			// Draw farm fields.
			if(block.m_isPartOfFarmField.contains(*m_window.getFaction()))
				colorOnBlock(block, displayData::farmFieldColor);
			// Draw cliff edge, ramp, or stairs below floor.
			blockWallsFromNextLevelDown(block);
		}
		else if(block.getBlockBelow() && block.getBlockBelow()->m_hasFluids.getTotalVolume() > displayData::minimumFluidVolumeToSeeFromAboveLevelRatio * Config::maxBlockVolume)
		{
			const FluidType& fluidType = block.getBlockBelow()->m_hasFluids.getFluidTypeWithMostVolume();
			const sf::Color color = displayData::fluidColors.at(&fluidType);
			static sf::Sprite sprite = sprites::make("fluidSurface").first;
			// TODO: Give the shore line some feeling of depth somehow.
			spriteOnBlock(block, sprite, &color);
		}
	}
}
void Draw::blockWallCorners(const Block& block)
{
	if(block.isSolid() && block.getBlockWest() && !block.getBlockWest()->isSolid() && block.getBlockSouth() && !block.getBlockSouth()->isSolid())
	{
		const sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		auto pair = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
		auto sprite = pair.first;
		sprite.setPosition((static_cast<float>(block.m_x) - 0.21f) * (float)m_window.m_scale, ((static_cast<float>(block.m_y) + 0.48f) * (float)m_window.m_scale));
		sprite.setScale(scaleRatio, scaleRatio);
		sprite.setColor(color);
		sprite.setRotation(45);
		m_window.getRenderWindow().draw(sprite);
	}
}
void Draw::blockWalls(const Block& block)
{
	if(block.isSolid())
	{
		if(block.getBlockSouth() && block.getBlockSouth()->m_visible)
		{
			const sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y + 1) * m_window.m_scale));
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		if(block.getBlockWest() && block.getBlockWest()->m_visible)
		{
			const sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y) * m_window.m_scale));
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			sprite.setRotation(90);
			m_window.getRenderWindow().draw(sprite);
		}
	}
}
void Draw::blockWallTops(const Block& block)
{
	if(block.isSolid())
	{
		auto adjacentPredicate = [](const Block* adjacent){ 
			return adjacent && adjacent->m_visible && !adjacent->isSolid() && !adjacent->m_hasBlockFeatures.contains(BlockFeatureType::stairs) && !adjacent->m_hasBlockFeatures.contains(BlockFeatureType::ramp);
		};
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		const sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
		float offset = displayData::wallTopOffsetRatio * m_window.m_scale;
		if(adjacentPredicate(block.getBlockNorth()))
		{
			auto pair = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setRotation(180);
			sprite.setPosition(static_cast<float>((block.m_x + 1) * m_window.m_scale), static_cast<float>((block.m_y) * m_window.m_scale) + offset);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		if(adjacentPredicate(block.getBlockEast()))
		{
			auto pair = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setRotation(270);
			sprite.setPosition(static_cast<float>((block.m_x + 1) * m_window.m_scale) - offset, static_cast<float>((block.m_y + 1) * m_window.m_scale));
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		if(adjacentPredicate(block.getBlockWest()))
		{
			auto pair = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setRotation(90);
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale) + offset, static_cast<float>((block.m_y) * m_window.m_scale));
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		if(adjacentPredicate(block.getBlockSouth()))
		{
			auto pair = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
			auto sprite = pair.first;
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y + 1) * m_window.m_scale) - offset);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
	}
}
void Draw::blockFeaturesAndFluids(const Block& block)
{
	// Block Features
	//TODO: Draw order.
	if(!block.m_hasBlockFeatures.empty())
	{
		for(const BlockFeature& blockFeature : block.m_hasBlockFeatures.get())
		{
			sf::Color* color = &displayData::materialColors.at(blockFeature.materialType);
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
				Facing facing = rampOrStairsFacing(block);
				stairs.setRotation(facing * 90);
				stairs.setOrigin(16,19);
				spriteOnBlockCentered(block, stairs, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::ramp)
			{
				static sf::Sprite ramp = getCenteredSprite("ramp");
				Facing facing = rampOrStairsFacing(block);
				ramp.setRotation(facing * 90);
				ramp.setOrigin(16,19);
				spriteOnBlockCentered(block, ramp, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::floodGate)
			{
				static sf::Sprite floodGate = getCenteredSprite("floodGate");
				// Default floodGate image leads north-south, maybe rotate.
				if(!block.getBlockNorth() || block.getBlockNorth()->isSolid() || !block.getBlockSouth() || block.getBlockSouth()->isSolid())
					floodGate.setRotation(90);
				else
					floodGate.setRotation(0);
				spriteOnBlockCentered(block, floodGate, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::fortification)
			{
				static sf::Sprite fortification = getCenteredSprite("fortification");
				// Default fortification image leads north-south, maybe rotate.
				if(!block.getBlockNorth() || block.getBlockNorth()->isSolid() || !block.getBlockNorth()->m_hasShapes.canStandIn() || !block.getBlockSouth() || block.getBlockSouth()->isSolid() || !block.getBlockSouth()->m_hasShapes.canStandIn())
					fortification.setRotation(90);
				else
					fortification.setRotation(0);
				spriteOnBlockCentered(block, fortification, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::door)
			{
				static sf::Sprite door = getCenteredSprite("door");
				// Default door image leads north-south, maybe rotate.
				if(!block.getBlockNorth() || block.getBlockNorth()->isSolid() || !block.getBlockNorth()->m_hasShapes.canStandIn() || !block.getBlockSouth() || block.getBlockSouth()->isSolid() || !block.getBlockSouth()->m_hasShapes.canStandIn())
					door.setRotation(90);
				else
					door.setRotation(0);
				spriteOnBlockCentered(block, door, color);
			}
			else if(blockFeature.blockFeatureType == &BlockFeatureType::flap)
			{
				static sf::Sprite flap = getCenteredSprite("flap");
				// Default flap image leads north-south, maybe rotate.
				if(!block.getBlockNorth() || block.getBlockNorth()->isSolid() || !block.getBlockNorth()->m_hasShapes.canStandIn() || !block.getBlockSouth() || block.getBlockSouth()->isSolid() || !block.getBlockSouth()->m_hasShapes.canStandIn())
					flap.setRotation(90);
				else
					flap.setRotation(0);
				spriteOnBlockCentered(block, flap, color);
			}
		}
	}
	else if(block.getBlockBelow() && !block.getBlockBelow()->isSolid())
	{
		// Show tops of stairs and ramps from next level down.
		const Block& below = *block.getBlockBelow();
		if(below.m_hasBlockFeatures.contains(BlockFeatureType::stairs))
		{
			const BlockFeature& blockFeature = *below.m_hasBlockFeatures.atConst(BlockFeatureType::stairs);
			sf::Color* color = &displayData::materialColors.at(blockFeature.materialType);
			static sf::Sprite stairs = getCenteredSprite("stairs");
			Facing facing = rampOrStairsFacing(below);
			stairs.setRotation(facing * 90);
			stairs.setOrigin(16,19);
			stairs.setTextureRect({0,0,32,16});
			spriteOnBlockCentered(block, stairs, color);
		}
		else if(below.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
		{
			const BlockFeature& blockFeature = *below.m_hasBlockFeatures.atConst(BlockFeatureType::ramp);
			sf::Color* color = &displayData::materialColors.at(blockFeature.materialType);
			static sf::Sprite ramp = getCenteredSprite("ramp");
			Facing facing = rampOrStairsFacing(below);
			ramp.setRotation(facing * 90);
			ramp.setOrigin(16,19);
			ramp.setTextureRect({0,0,32,16});
			spriteOnBlockCentered(block, ramp, color);
		}

	}
	// Fluids
	if(block.m_hasFluids.getTotalVolume())
	{
		const FluidType& fluidType = block.m_hasFluids.getFluidTypeWithMostVolume();
		CollisionVolume volume = block.m_hasFluids.volumeOfFluidTypeContains(fluidType);
		const sf::Color color = displayData::fluidColors.at(&fluidType);
		colorOnBlock(block, color);
		stringOnBlock(block, std::to_wstring(volume), sf::Color::Black);
	}
}
void Draw::blockWallsFromNextLevelDown(const Block& block)
{
	if(!block.getBlockBelow() || !block.getBlockBelow()->isSolid())
		return;
	const Block& below = *block.getBlockBelow();
	if(below.getBlockSouth() && !below.getBlockSouth()->isSolid())
	{
		const Block& belowSouth = *below.getBlockSouth();
		sf::Sprite* sprite;
		const MaterialType* materialType;
		if(block.isConstructed())
		{
			static sf::Sprite blockWall  = getCenteredSprite("blockWall");
			sprite = &blockWall;
			materialType = &below.getSolidMaterial();
		}
		else
		{
			static sf::Sprite roughWall  = getCenteredSprite("roughWall");
			sprite = &roughWall;
			materialType = &below.getSolidMaterial();
		}
		sprite->setTextureRect({0, 0, 32, 18});
		assert(materialType);
		assert(displayData::materialColors.contains(materialType));
		sf::Color color = displayData::materialColors.at(materialType);
		sf::Vector2f position{(((float)belowSouth.m_x + 0.5f) * (float)m_window.m_scale), (((float)belowSouth.m_y + 0.5f - displayData::wallOffset) * (float)m_window.m_scale)};
		spriteAt(*sprite, position, &color);
	}
	if(below.getBlockWest() && !below.getBlockWest()->isSolid())
	{
		const Block& belowWest = *below.getBlockWest();
		static sf::Sprite* sprite;
		const MaterialType* materialType;
		if(block.isConstructed())
		{
			static sf::Sprite blockWall  = getCenteredSprite("blockWall");
			sprite = &blockWall;
			materialType = &below.getSolidMaterial();
		}
		else
		{
			static sf::Sprite roughWall  = getCenteredSprite("roughWall");
			sprite = &roughWall;
			materialType = &below.getSolidMaterial();
		}
		sprite->setTextureRect({0, 0, 32, 18});
		sprite->setRotation(90);
		assert(materialType);
		assert(displayData::materialColors.contains(materialType));
		sf::Color color = displayData::materialColors.at(materialType);
		sf::Vector2f position{(((float)belowWest.m_x + 0.5f + displayData::wallOffset) * (float)m_window.m_scale), (((float)belowWest.m_y + 0.5f) * (float)m_window.m_scale)};
		spriteAt(*sprite, position, &color);
	}
}
void Draw::validOnBlock(const Block& block)
{
	colorOnBlock(block, {0, 255, 0, 122});
}
void Draw::invalidOnBlock(const Block& block)
{
	colorOnBlock(block, {255, 0, 0, 122});
}
void Draw::colorOnBlock(const Block& block, const sf::Color color)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, m_window.m_scale));
	square.setFillColor(color);
	square.setPosition(static_cast<float>(block.m_x * m_window.m_scale), static_cast<float>(block.m_y * m_window.m_scale));
	m_window.getRenderWindow().draw(square);
}
void Draw::designated(const Block& block)
{
	BlockDesignation designation = block.m_hasDesignations.getDisplayDesignation(*m_window.m_faction);
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
void Draw::craftLocation(const Block& block)
{
	const CraftStepTypeCategory* craftDisplay = block.m_area->m_hasCraftingLocationsAndJobs.at(*m_window.getFaction()).getDisplayStepTypeCategoryForLocation(block);
	if(craftDisplay)
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
void Draw::spriteOnBlockWithScaleCentered(const Block& block, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = m_window.m_scale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	sprite.setPosition(((float)block.m_x + 0.5f) * windowScale, ((float)block.m_y + 0.5f) * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteOnBlockWithScale(const Block& block, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = m_window.m_scale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	sprite.setPosition((float)block.m_x * windowScale, (float)block.m_y * windowScale);
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
void Draw::spriteOnBlock(const Block& block, sf::Sprite& sprite, const sf::Color* color)
{
	sf::Vector2f position{(float)(block.m_x * m_window.m_scale), (float)(block.m_y * m_window.m_scale)};
	spriteAt(sprite, position, color);
}
void Draw::spriteOnBlockCentered(const Block& block, sf::Sprite& sprite, const sf::Color* color)
{
	sf::Vector2f position{(((float)block.m_x + 0.5f) * (float)m_window.m_scale), (((float)block.m_y + 0.5f) * (float)m_window.m_scale)};
	spriteAt(sprite, position, color);
}
void Draw::imageOnBlock(const Block& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	spriteOnBlock(block, pair.first, color);
}
// By default images are top aligned.
void Draw::imageOnBlockWestAlign(const Block& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(270);
	spriteOnBlock(block, pair.first, color);
}
void Draw::imageOnBlockEastAlign(const Block& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(90);
	spriteOnBlock(block, pair.first, color);
}
void Draw::imageOnBlockSouthAlign(const Block& block, std::string name, const sf::Color* color)
{
	auto pair = sprites::make(name);
	pair.first.setRotation(180);
	spriteOnBlock(block, pair.first, color);
}
void Draw::progressBarOnBlock(const Block& block, Percent progress)
{
	float scaledUnit = getScaledUnit();
	sf::RectangleShape outline(sf::Vector2f(m_window.m_scale, scaledUnit * (2 + displayData::progressBarThickness)));
	outline.setFillColor(displayData::progressBarOutlineColor);
	outline.setPosition(static_cast<float>(block.m_x * m_window.m_scale), static_cast<float>(block.m_y * m_window.m_scale));
	m_window.getRenderWindow().draw(outline);
	float progressWidth = util::scaleByPercent(m_window.m_scale, progress) - (scaledUnit * 2);
	sf::RectangleShape rectangle(sf::Vector2f(progressWidth, scaledUnit * displayData::progressBarThickness));
	rectangle.setFillColor(displayData::progressBarColor);
	rectangle.setPosition(static_cast<float>(block.m_x * m_window.m_scale) + scaledUnit, static_cast<float>(block.m_y * m_window.m_scale) + scaledUnit);
	m_window.getRenderWindow().draw(rectangle);
}
void Draw::selected(Block& block) { outlineOnBlock(block, displayData::selectColor); }
void Draw::outlineOnBlock(const Block& block, const sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale - (thickness*2), m_window.m_scale - (thickness*2)));
	square.setFillColor(sf::Color::Transparent);
	square.setOutlineColor(color);
	square.setOutlineThickness(thickness);
	square.setPosition(static_cast<float>(block.m_x * m_window.m_scale) + thickness, static_cast<float>(block.m_y * m_window.m_scale) + thickness);
	m_window.getRenderWindow().draw(square);
}
void Draw::stringOnBlock(const Block& block, std::wstring string, const sf::Color color, float offsetX, float offsetY )
{
	return stringAtPosition(string, {
		static_cast<float>(block.m_x + offsetX) * (float)m_window.m_scale,
		static_cast<float>(block.m_y + offsetY) * (float)m_window.m_scale
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
void Draw::nonGroundCoverPlant(const Block& block)
{
	if(!block.m_hasPlant.exists())
		return;
	const Plant& plant = block.m_hasPlant.get();
	PlantSpeciesDisplayData& display = displayData::plantData.at(&plant.m_plantSpecies);
	// Ground cover plants are drawn with floors.
	if(display.groundCover)
		return;
	if(plant.m_plantSpecies.isTree && plant.m_blocks.size() != 1)
	{
		if(block == *plant.m_location && plant.m_blocks.size() > 2)
		{
			static sf::Sprite trunk = getCenteredSprite("trunk");
			spriteOnBlockCentered(block, trunk);
		}
		else 
		{
			if(block.m_x == plant.m_location->m_x && block.m_y == plant.m_location->m_y)
			{
				if(block.getBlockAbove() && block.getBlockAbove()->m_hasPlant.exists() && block.getBlockAbove()->m_hasPlant.get() == plant)
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
				float angle = 45.f * block.facingToSetWhenEnteringFromIncludingDiagonal(*plant.m_location);
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
		float scale = plant.m_blocks.size() == 1 ? ((float)plant.getGrowthPercent() / 100.f) : 1;
		spriteOnBlockWithScaleCentered(block, pair.first, scale);
		if(m_window.getSelectedPlants().contains(const_cast<Plant*>(&plant)))
			outlineOnBlock(block, displayData::selectColor);
	}
}
// Item.
void Draw::item(const Block& block)
{
	if(block.m_hasItems.empty())
		return;
	Item& itemToDraw = *block.m_hasItems.getAll().front();
	item(itemToDraw, blockToPositionCentered(block));
}
void Draw::item(const Item& item, sf::Vector2f position)
{
	ItemTypeDisplayData& display = displayData::itemData.at(&item.m_itemType);
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	const sf::Color materialColor = displayData::materialColors.at(&item.m_materialType);
	sprite.setColor(materialColor);
	spriteAtWithScale(sprite, position, display.scale);
}
void Draw::itemOverlay(const Item& item, sf::Vector2f position)
{
	if(item.getQuantity() != 1)
		stringAtPosition(std::to_wstring(item.getQuantity()), position, sf::Color::White);
}
void Draw::actorOverlay(const Actor& actor)
{
	sf::Vector2f location = blockToPosition(*actor.m_location);
	location.y -= (float)m_window.m_scale / 3.5f;
	if(!actor.m_mustSleep.isAwake())
	{
		sf::Vector2f sleepTextLocation = location;
		sleepTextLocation.y += m_window.m_scale / 3.5f;
		stringAtPosition(L"zzz", sleepTextLocation, sf::Color::Cyan);
	}
}
void Draw::itemOverlay(const Block& block)
{
	if(block.m_hasItems.empty())
		return;
	Item& item = *block.m_hasItems.getAll().front();
	if(block.m_hasItems.getAll().size() > 1)
		stringOnBlock(block, std::to_wstring(block.m_hasItems.getAll().size()), sf::Color::Magenta);
	else if(item.getQuantity() != 1)
		stringOnBlock(block, std::to_wstring(item.getQuantity()), sf::Color::White);
	if(m_window.getSelectedItems().contains(const_cast<Item*>(&item)))
		outlineOnBlock(block, displayData::selectColor);
	if(m_window.getFaction() && !item.m_canBeStockPiled.contains(*m_window.getFaction()))
		inaccessableSymbol(block);
}
// Actor.
void Draw::singleTileActor(const Actor& actor)
{
	Block& block = *actor.m_location;
	AnimalSpeciesDisplayData& display = displayData::actorData.at(&actor.m_species);
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	spriteOnBlockWithScaleCentered(block, sprite, ((float)actor.m_canGrow.growthPercent() + 10.f) / 110.f);
	if(actor.m_canPickup.exists())
	{
		HasShape& isCarrying = *const_cast<Actor&>(actor).m_canPickup.getCarrying();
		if(isCarrying.isItem())
		{
			sf::Vector2f position = blockToPositionCentered(block);
			position.y += getScaledUnit() * 4;
			Item& itemToDraw = static_cast<Item&>(isCarrying);
			item(itemToDraw, position);
		}
		else 
		{
			assert(isCarrying.isActor());
			// TODO: Draw actors being carried.
		}
	}
	if(m_window.getSelectedActors().contains(const_cast<Actor*>(&actor)))
		outlineOnBlock(block, displayData::selectColor);
}
void Draw::multiTileActor(const Actor& actor)
{
	AnimalSpeciesDisplayData& display = displayData::actorData.at(&actor.m_species);
	auto [sprite, origin] = sprites::make(display.image);
	if(actor.m_shape->displayScale == 1)
	{
		Block& location = m_window.m_area->getBlock(actor.m_location->m_x, actor.m_location->m_y, m_window.m_z);
		spriteOnBlock(location, sprite, &display.color);
		multiTileBorder(actor.m_blocks, displayData::actorOutlineColor, 1);
	}
	else
	{
		Block* topLeft = nullptr;
		for(Block* block : actor.m_blocks)
			if(block->m_z == m_window.m_z)
				if(!topLeft || block->m_x < topLeft->m_x || block->m_y < topLeft->m_y)
					topLeft = block;
		spriteOnBlockWithScale(*topLeft, sprite, actor.m_shape->displayScale, &display.color);
	}
}
void Draw::multiTileBorder(std::unordered_set<Block*> blocks, sf::Color color, float thickness)
{
	for(Block* block : blocks)
		if(block->m_z == m_window.m_z)
		{
			for(Block* adjacent : block->getAdjacentOnSameZLevelOnly())
				if(!blocks.contains(adjacent))
				{
					Facing facing = adjacent->facingToSetWhenEnteringFrom(*block);
					borderSegmentOnBlock(*block, facing, color, thickness);
				}
		}
}
void Draw::borderSegmentOnBlock(const Block& block, Facing facing, sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, thickness));
	square.setFillColor(color);
	switch(facing)
	{
		case 0:
			// do nothing
			square.setPosition(
				static_cast<float>((block.m_x) * m_window.m_scale), 
				static_cast<float>(block.m_y * m_window.m_scale)
			);
			break;
		case 1:
			square.setRotation(90);
			square.setPosition(
				static_cast<float>((block.m_x + 1) * m_window.m_scale) - thickness, 
				static_cast<float>(block.m_y * m_window.m_scale)
			);
		break;
		case 2:
			square.setPosition(
				static_cast<float>(block.m_x * m_window.m_scale), 
				static_cast<float>((block.m_y + 1) * m_window.m_scale) - thickness
			);
			break;

		case 3:
			square.setRotation(90);
			square.setPosition(
				static_cast<float>(block.m_x * m_window.m_scale), 
				static_cast<float>(block.m_y * m_window.m_scale)
			);
			break;
	}
	m_window.getRenderWindow().draw(square);
}
Facing Draw::rampOrStairsFacing(const Block& block) const
{
	static auto canConnectToAbove = [](const Block& block) -> bool{ 
		return block.getBlockAbove() && !block.m_hasBlockFeatures.contains(BlockFeatureType::stairs) && !block.m_hasBlockFeatures.contains(BlockFeatureType::ramp) && block.getBlockAbove()->m_hasShapes.canStandIn();
	};
	if(!block.getBlockAbove())
		return 0;
	Facing backup = 0;
	if(block.getBlockNorth() && canConnectToAbove(*block.getBlockNorth()))
	{
		if(!block.getBlockSouth()->isSolid() && block.getBlockSouth()->m_hasShapes.canStandIn() &&
			!block.getBlockSouth()->m_hasBlockFeatures.contains(BlockFeatureType::stairs) &&
			!block.getBlockSouth()->m_hasBlockFeatures.contains(BlockFeatureType::ramp)
		)
			return 0;
	}
	if(block.getBlockEast() && canConnectToAbove(*block.getBlockEast()))
	{
		if(!block.getBlockWest()->isSolid() && block.getBlockWest()->m_hasShapes.canStandIn())
			return 1;
		else
			backup = 1;
	}
	if(block.getBlockSouth() && canConnectToAbove(*block.getBlockSouth()))
	{
		if(!block.getBlockNorth()->isSolid() && block.getBlockNorth()->m_hasShapes.canStandIn() &&
			!block.getBlockNorth()->m_hasBlockFeatures.contains(BlockFeatureType::stairs) &&
			!block.getBlockNorth()->m_hasBlockFeatures.contains(BlockFeatureType::ramp)
		)
			return 2;
		else
			backup = 2;
	}
	if(block.getBlockWest() && canConnectToAbove(*block.getBlockWest()))
	{
		if(!block.getBlockEast()->isSolid() && block.getBlockEast()->m_hasShapes.canStandIn())
			return 3;
		else
			backup = 3;
	}
	return backup;
}
sf::Vector2f Draw::blockToPosition(const Block& block) const
{
	return {static_cast<float>(block.m_x * m_window.m_scale), static_cast<float>(block.m_y * m_window.m_scale)};
}
sf::Vector2f Draw::blockToPositionCentered(const Block& block) const
{
	return {static_cast<float>((block.m_x + 0.5) * m_window.m_scale), static_cast<float>((block.m_y + 0.5) * m_window.m_scale)};
}
void Draw::accessableSymbol(const Block& block)
{
	stringOnBlock(block, L"o", sf::Color::Green, 0.1, 0.6);
}
void Draw::inaccessableSymbol(const Block& block)
{
	stringOnBlock(block, L"x", sf::Color::Red, 0.1, 0.6);
}
float Draw::getScaledUnit() const { return float(m_window.m_scale) / float(displayData::defaultScale); }
