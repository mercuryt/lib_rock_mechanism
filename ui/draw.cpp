#include "draw.h"
#include "blockFeature.h"
#include "config.h"
#include "designations.h"
#include "sprite.h"
#include "util.h"
#include "window.h"
#include "../engine/block.h"
#include "../engine/actor.h"
#include "../engine/item.h"
#include "../engine/plant.h"
#include "displayData.h"
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/System/Vector2.hpp>
// Block.
void Draw::blockFloor(const Block& block)
{
	if(block.isSolid())
	{
		for(Block* adjacent : block.m_adjacentsVector)
			if(m_window.m_editMode || adjacent->m_visible)
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
			// Draw cliff edge below floor.
			// TODO: West side edge.
			if(block.getBlockBelow()->getBlockSouth() && !block.getBlockBelow()->getBlockSouth()->isSolid())
			{
				float offset = displayData::wallOffsetRatio * m_window.m_scale;
				const sf::Color color = displayData::materialColors.at(&block.getBlockBelow()->getSolidMaterial());
				float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
				sf::Sprite* sprite;
				if(block.isConstructed())
				{
					static sf::Sprite blockSprite = sprites::make("blockWall").first;
					sprite  = &blockSprite;
				}
				else 
				{
					static sf::Sprite roughSprite = sprites::make("roughWall").first;
					sprite = &roughSprite;
				}
				sprite->setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y + 1) * m_window.m_scale) + offset);
				sprite->setScale(scaleRatio, scaleRatio);
				sprite->setColor(color);
				m_window.getRenderWindow().draw(*sprite);
			}
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
		}
		else if(block.getBlockBelow() && block.getBlockBelow()->m_totalFluidVolume > displayData::minimumFluidVolumeToSeeFromAboveLevelRatio * Config::maxBlockVolume)
		{
			const FluidType& fluidType = block.getBlockBelow()->getFluidTypeWithMostVolume();
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
		float offset = displayData::wallOffsetRatio * m_window.m_scale;
		const sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		auto pair = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
		auto sprite = pair.first;
		sprite.setPosition((static_cast<float>(block.m_x) - 0.21f) * (float)m_window.m_scale, ((static_cast<float>(block.m_y) + 0.48f) * (float)m_window.m_scale) + offset);
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
			float offset = displayData::wallOffsetRatio * m_window.m_scale;
			const sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y + 1) * m_window.m_scale) + offset);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		if(block.getBlockWest() && block.getBlockWest()->m_visible)
		{
			float offset = displayData::wallOffsetRatio * m_window.m_scale;
			const sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			auto pair = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
			auto sprite = pair.first;
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y) * m_window.m_scale) + offset);
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
	for(const BlockFeature& blockFeature : block.m_hasBlockFeatures.get())
	{
		if(blockFeature.blockFeatureType == &BlockFeatureType::hatch)
			imageOnBlock(block, "hatch", &displayData::materialColors.at(blockFeature.materialType));
		else if(blockFeature.blockFeatureType == &BlockFeatureType::floorGrate)
			imageOnBlock(block, "floorGrate", &displayData::materialColors.at(blockFeature.materialType));
		else if(blockFeature.blockFeatureType == &BlockFeatureType::stairs)
			imageOnBlock(block, "stairs", &displayData::materialColors.at(blockFeature.materialType));
		else if(blockFeature.blockFeatureType == &BlockFeatureType::ramp)
			imageOnBlock(block, "ramp", &displayData::materialColors.at(blockFeature.materialType));
		else if(blockFeature.blockFeatureType == &BlockFeatureType::floodGate)
			imageOnBlock(block, "floodGate", &displayData::materialColors.at(blockFeature.materialType));
		else if(blockFeature.blockFeatureType == &BlockFeatureType::fortification)
			imageOnBlock(block, "fortification", &displayData::materialColors.at(blockFeature.materialType));
		else if(blockFeature.blockFeatureType == &BlockFeatureType::door)
			imageOnBlock(block, "door", &displayData::materialColors.at(blockFeature.materialType));
		else if(blockFeature.blockFeatureType == &BlockFeatureType::flap)
			imageOnBlock(block, "flap", &displayData::materialColors.at(blockFeature.materialType));
	}
	// Fluids
	if(block.m_totalFluidVolume)
	{
		const FluidType& fluidType = block.getFluidTypeWithMostVolume();
		Volume volume = block.m_fluids.at(&fluidType).first;
		const sf::Color color = displayData::fluidColors.at(&fluidType);
		colorOnBlock(block, color);
		stringOnBlock(block, std::to_wstring(volume), sf::Color::Black);
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
sf::Sprite Draw::getCenteredSprite(std::string name)
{
	auto pair = sprites::make(name);
	pair.first.setOrigin(pair.second);
	return pair.first;
}
// Origin is assumed to already be set in sprite on block.
void Draw::spriteOnBlockWithScale(const Block& block, sf::Sprite& sprite, float scale, const sf::Color* color)
{
	float windowScale = m_window.m_scale;
	scale = scale * (windowScale / (float)displayData::defaultScale);
	sprite.setScale(scale, scale);
	sprite.setPosition(((float)block.m_x + 0.5f) * windowScale, ((float)block.m_y + 0.5f) * windowScale);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::spriteAt(sf::Sprite& sprite, sf::Vector2f position, const sf::Color* color)
{
	sprite.setPosition(position);
	float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
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
void Draw::stringOnBlock(const Block& block, std::wstring string, const sf::Color color)
{
	sf::Text text;
	text.setFont(m_window.m_font);
	text.setFillColor(color);
	text.setCharacterSize(m_window.m_scale * displayData::ratioOfScaleToFontSize);
	text.setString(string);
	text.setPosition((static_cast<float>((block.m_x) + 0.5f) * (float)m_window.m_scale), static_cast<float>((block.m_y) * (float)m_window.m_scale));
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
	auto pair = sprites::make(display.image);
	//TODO: color
	pair.first.setOrigin(pair.second);
	float scale = plant.m_blocks.size() == 1 ? ((float)plant.getGrowthPercent() / 100.f) : 1;
	spriteOnBlockWithScale(block, pair.first, scale);
	if(m_window.getSelectedPlants().contains(const_cast<Plant*>(&plant)))
		outlineOnBlock(block, displayData::selectColor);
}
// Item.
void Draw::item(const Block& block)
{
	if(block.m_hasItems.empty())
		return;
	Item& item = *block.m_hasItems.getAll().front();
	ItemTypeDisplayData& display = displayData::itemData.at(&item.m_itemType);
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	const sf::Color materialColor = displayData::materialColors.at(&item.m_materialType);
	sprite.setColor(materialColor);
	spriteOnBlockWithScale(block, sprite, display.scale);
	if(block.m_hasItems.getAll().size() > 1)
		stringOnBlock(block, std::to_wstring(block.m_hasItems.getAll().size()), sf::Color::Black);
	else if(item.getQuantity() != 1)
		stringOnBlock(block, std::to_wstring(item.getQuantity()), sf::Color::White);
	if(m_window.getSelectedItems().contains(const_cast<Item*>(&item)))
		outlineOnBlock(block, displayData::selectColor);
}
// Actor.
void Draw::singleTileActor(const Actor& actor)
{
	Block& block = *actor.m_location;
	AnimalSpeciesDisplayData& display = displayData::actorData.at(&actor.m_species);
	auto [sprite, origin] = sprites::make(display.image);
	sprite.setOrigin(origin);
	sprite.setColor(display.color);
	spriteOnBlockWithScale(block, sprite, ((float)actor.m_canGrow.growthPercent() + 10.f) / 110.f);
	if(m_window.getSelectedActors().contains(const_cast<Actor*>(&actor)))
		outlineOnBlock(block, displayData::selectColor);
}
void Draw::multiTileActor(const Actor& actor)
{
	(void)actor;
	assert(false);
	//TODO
}
