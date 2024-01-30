#include "draw.h"
#include "sprite.h"
#include "window.h"
#include "../engine/block.h"
#include "../engine/actor.h"
#include "../engine/item.h"
#include "../engine/plant.h"
#include "displayData.h"
// Block.
void Draw::block(const Block& block)
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
		if(block.getBlockSouth() && block.getBlockSouth()->m_visible)
			imageOnBlock(block, "roughWall", &displayData::materialColors.at(&block.getSolidMaterial()));
	}
	else
	{
		if(block.getBlockBelow() && block.getBlockBelow()->isSolid())
		{
			// Draw floor.
			imageOnBlock(block, "roughFloor", &displayData::materialColors.at(&block.getBlockBelow()->getSolidMaterial()));
		}
		// Block Features
		//TODO: Draw order.
		for(const BlockFeature& blockFeature : block.m_hasBlockFeatures.get())
		{
			std::wstring string = displayData::blockFeatureSymbols.at(blockFeature.blockFeatureType);
			sf::Color color = displayData::materialColors.at(blockFeature.materialType);
			stringOnBlock(block, string, color);
		}
		// Fluids
		if(block.m_totalFluidVolume)
		{
			const FluidType& fluidType = block.getFluidTypeWithMostVolume();
			Volume volume = block.m_fluids.at(&fluidType).first;
			sf::Color color = displayData::fluidColors.at(&fluidType);
			stringOnBlock(block, std::to_wstring(volume), color);
		}
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
void Draw::imageOnBlock(const Block& block, std::string name, [[maybe_unused]] sf::Color* color)
{
	sf::Sprite sprite = sprites::make(name);
	sprite.setPosition(static_cast<float>(block.m_x * m_window.m_scale), static_cast<float>(block.m_y * m_window.m_scale));
	float scaleRatio = (float)m_window.m_scale / (float)displayData::defaultScale;
	sprite.setScale(scaleRatio, scaleRatio);
	//if(color)
		//sprite.setColor(*color);
	m_window.getRenderWindow().draw(sprite);
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
	text.setOrigin(0.5, 0.5);
	text.setPosition(static_cast<float>(block.m_x * ((float)m_window.m_scale + 0.5f)), static_cast<float>(block.m_y * ((float)m_window.m_scale + 0.5f)));
	m_window.getRenderWindow().draw(text);
}
// Actor.
void Draw::actor(const Actor& actor)
{
	std::wstring str = displayData::actorSymbols.at(&actor.m_species);
	// TODO: Toggle team colors, job colors, weapon border.
	sf::Color color = displayData::actorColors.at(&actor.m_species);
	stringOnBlock(*actor.m_location, str, color);
	if(m_window.m_selectedActors.contains(&const_cast<Actor&>(actor)))
		for(Block* occupied : actor.m_blocks)
			outlineOnBlock(*occupied, displayData::selectColor);
}
// Item.
void Draw::item(const Item& item)
{
	std::wstring str = displayData::itemSymbols.at(&item.m_itemType);
	sf::Color color = displayData::materialColors.at(&item.m_materialType);
	stringOnBlock(*item.m_location, str, color);
	if(m_window.m_selectedItems.contains(&const_cast<Item&>(item)))
		for(Block* occupied : item.m_blocks)
			outlineOnBlock(*occupied, displayData::selectColor);
}
// Plant.
void Draw::plant(const Plant& plant)
{
	std::wstring str = displayData::plantSymbols.at(&plant.m_plantSpecies);
	stringOnBlock(*plant.m_location, str, sf::Color::Green);
	if(m_window.m_selectedPlants.contains(&const_cast<Plant&>(plant)))
		for(Block* occupied : plant.m_blocks)
			outlineOnBlock(*occupied, displayData::selectColor);
}
