#include "draw.h"
#include "blockFeature.h"
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
// Block.
void Draw::blockFloor(const Block& block)
{
	if(block.isSolid())
	{
		for(Block* adjacent : block.m_adjacentsVector)
			if(adjacent->m_visible)
			{
				assert(displayData::materialColors.contains(&block.getSolidMaterial()));
				colorOnBlock(block, displayData::materialColors.at(&block.getSolidMaterial()));
				break;
			}
	}
	else if(block.m_visible)
		if(block.getBlockBelow() && block.getBlockBelow()->isSolid())
		{
			// Draw cliff edge below floor.
			// TODO: West side edge.
			if(block.getBlockBelow()->getBlockSouth() && !block.getBlockBelow()->getBlockSouth()->isSolid())
			{
				float offset = displayData::wallOffsetRatio * m_window.m_scale;
				sf::Color color = displayData::materialColors.at(&block.getBlockBelow()->getSolidMaterial());
				float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
				sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
				sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y + 1) * m_window.m_scale) + offset);
				sprite.setScale(scaleRatio, scaleRatio);
				sprite.setColor(color);
				m_window.getRenderWindow().draw(sprite);
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
					return;
				}
			}
			std::string name = block.isConstructed() ? "blockFloor" : "roughFloor";
			imageOnBlock(block, "roughFloor", &displayData::materialColors.at(&block.getBlockBelow()->getSolidMaterial()));
		}
}
void Draw::blockWallCorners(const Block& block)
{
	if(block.isSolid() && block.getBlockWest() && !block.getBlockWest()->isSolid() && block.getBlockSouth() && !block.getBlockSouth()->isSolid())
	{
		float offset = displayData::wallOffsetRatio * m_window.m_scale;
		sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
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
			sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale), static_cast<float>((block.m_y + 1) * m_window.m_scale) + offset);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		if(block.getBlockWest() && block.getBlockWest()->m_visible)
		{
			float offset = displayData::wallOffsetRatio * m_window.m_scale;
			sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
			float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
			sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWall" : "roughWall");
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
		const Block* adjacent = block.getBlockNorth();
		float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
		sf::Color color = displayData::materialColors.at(&block.getSolidMaterial());
		float offset = displayData::wallTopOffsetRatio * m_window.m_scale;
		if(adjacent && adjacent->m_visible && !adjacent->isSolid())
		{
			sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
			sprite.setRotation(180);
			sprite.setPosition(static_cast<float>((block.m_x + 1) * m_window.m_scale), static_cast<float>((block.m_y) * m_window.m_scale) + offset);
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		adjacent = block.getBlockEast();
		if(adjacent && adjacent->m_visible && !adjacent->isSolid())
		{
			sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
			sprite.setRotation(270);
			sprite.setPosition(static_cast<float>((block.m_x + 1) * m_window.m_scale) - offset, static_cast<float>((block.m_y + 1) * m_window.m_scale));
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		adjacent = block.getBlockWest();
		if(adjacent && adjacent->m_visible && !adjacent->isSolid())
		{
			sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
			sprite.setRotation(90);
			sprite.setPosition(static_cast<float>((block.m_x) * m_window.m_scale) + offset, static_cast<float>((block.m_y) * m_window.m_scale));
			sprite.setScale(scaleRatio, scaleRatio);
			sprite.setColor(color);
			m_window.getRenderWindow().draw(sprite);
		}
		adjacent = block.getBlockSouth();
		if(adjacent && adjacent->m_visible && !adjacent->isSolid())
		{
			sf::Sprite& sprite = sprites::make(block.isConstructed() ? "blockWallTop" : "roughWallTop");
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
		sf::Color color = displayData::fluidColors.at(&fluidType);
		stringOnBlock(block, std::to_wstring(volume), color);
	}
	if(m_window.getSelectedBlocks().contains(block))
		outlineOnBlock(block, displayData::selectColor);
}
void Draw::validOnBlock(const Block& block)
{
	colorOnBlock(block, {0, 255, 0, 122});
}
void Draw::invalidOnBlock(const Block& block)
{
	colorOnBlock(block, {255, 0, 0, 122});
}
void Draw::colorOnBlock(const Block& block, sf::Color color)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, m_window.m_scale));
	square.setFillColor(color);
	square.setPosition(static_cast<float>(block.m_x * m_window.m_scale), static_cast<float>(block.m_y * m_window.m_scale));
	m_window.getRenderWindow().draw(square);
}
void Draw::spriteOnBlock(const Block& block, sf::Sprite& sprite, sf::Color* color)
{
	sprite.setPosition(static_cast<float>(block.m_x * m_window.m_scale), static_cast<float>(block.m_y * m_window.m_scale));
	float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
	sprite.setScale(scaleRatio, scaleRatio);
	if(color)
		sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
}
void Draw::imageOnBlock(const Block& block, std::string name, sf::Color* color)
{
	sf::Sprite& sprite = sprites::make(name);
	spriteOnBlock(block, sprite, color);
}
// By default images are top aligned.
void Draw::imageOnBlockWestAlign(const Block& block, std::string name, sf::Color* color)
{
	sf::Sprite& sprite = sprites::make(name);
	sprite.setRotation(270);
	spriteOnBlock(block, sprite, color);
}
void Draw::imageOnBlockEastAlign(const Block& block, std::string name, sf::Color* color)
{
	sf::Sprite& sprite = sprites::make(name);
	sprite.setRotation(90);
	spriteOnBlock(block, sprite, color);
}
void Draw::imageOnBlockSouthAlign(const Block& block, std::string name, sf::Color* color)
{
	sf::Sprite& sprite = sprites::make(name);
	sprite.setRotation(180);
	spriteOnBlock(block, sprite, color);
}
void Draw::outlineOnBlock(const Block& block, sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_window.m_scale, m_window.m_scale));
	square.setFillColor(sf::Color::Transparent);
	square.setOutlineColor(color);
	square.setOutlineThickness(thickness);
	square.setPosition(static_cast<float>(block.m_x * m_window.m_scale), static_cast<float>(block.m_y * m_window.m_scale));
	m_window.getRenderWindow().draw(square);
}
void Draw::stringOnBlock(const Block& block, std::wstring string, sf::Color color)
{
	sf::Text text;
	text.setFont(m_window.m_font);
	text.setFillColor(color);
	text.setCharacterSize(m_window.m_scale * displayData::ratioOfScaleToFontSize);
	text.setString(string);
	text.setPosition(static_cast<float>(block.m_x * ((float)m_window.m_scale + 0.5f)), static_cast<float>(block.m_y * ((float)m_window.m_scale + 0.5f)));
	m_window.getRenderWindow().draw(text);
}
// Plant.
void Draw::nonGroundCoverPlant(const Block& block)
{
	if(!block.m_hasPlant.exists())
		return;
	const Plant& plant = block.m_hasPlant.get();
	float scale = m_window.m_scale;
	PlantSpeciesDisplayData& display = displayData::plantData.at(&plant.m_plantSpecies);
	// Ground cover plants are drawn with floors.
	if(display.groundCover)
		return;
	sf::Sprite& sprite = sprites::make(display.image);
	//TODO: color
	sprite.setOrigin(scale / 2.f, scale / 1.4f);
	float scaleRatio = scale / (float)displayData::defaultScale;
	scaleRatio = util::scaleByPercent(scaleRatio, plant.getGrowthPercent() + 5);
	sprite.setScale(scaleRatio, scaleRatio);
	sprite.setPosition(((float)block.m_x + 0.5f) * scale, ((float)block.m_y + 0.5f) * scale);
	m_window.getRenderWindow().draw(sprite);
}
// Item.
void Draw::item(const Item& item)
{
	ItemTypeDisplayData& display = displayData::itemData.at(&item.m_itemType);
	sf::Sprite& sprite = sprites::make(display.image);
	sprite.setColor(display.color);
	//TODO: scale.
	m_window.getRenderWindow().draw(sprite);
}
// Actor.
void Draw::singleTileActor(const Actor& actor)
{
	AnimalSpeciesDisplayData& display = displayData::actorData.at(&actor.m_species);
	sf::Sprite& sprite = sprites::make(display.image);
	sprite.setColor(display.color);
	//TODO: scale.
	spriteOnBlock(*actor.m_location, sprite);
}
void Draw::multiTileActor(const Actor& actor)
{
	(void)actor;
	assert(false);
	//TODO
}
