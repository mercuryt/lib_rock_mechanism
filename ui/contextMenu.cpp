#include "contextMenu.h"
#include "blockFeature.h"
#include "config.h"
#include "designations.h"
#include "displayData.h"
#include "plant.h"
#include "widgets.h"
#include "window.h"
#include "../engine/actor.h"
#include "../engine/goTo.h"
#include "../engine/station.h"
#include "../engine/installItem.h"
#include "../engine/kill.h"
#include "../engine/uniform.h"
#include <TGUI/Widgets/HorizontalLayout.hpp>
// Menu segment
ContextMenuSegment::ContextMenuSegment(tgui::Group::Ptr overlayGroup) : 
	m_gameOverlayGroup(overlayGroup), m_panel(tgui::Panel::create()), m_grid(tgui::Grid::create()) 
{ 
	m_panel->add(m_grid);
	m_panel->setSize(tgui::bindWidth(m_grid) + 4, tgui::bindHeight(m_grid) + 4);
	m_grid->setPosition(2,2);
}
void ContextMenuSegment::add(tgui::Widget::Ptr widget, std::string id) 
{
	widget->setSize(width, height); 
	m_grid->add(widget, id); 
	m_grid->setWidgetCell(widget, m_count++, 1, tgui::Grid::Alignment::Center, tgui::Padding{1, 5});
}
ContextMenuSegment::~ContextMenuSegment() { m_gameOverlayGroup->remove(m_panel); }
// Menu
ContextMenu::ContextMenu(Window& window, tgui::Group::Ptr gameOverlayGroup) : 
	m_window(window), m_root(gameOverlayGroup), m_gameOverlayGroup(gameOverlayGroup)
{
	gameOverlayGroup->add(m_root.m_panel);
	m_root.m_panel->setVisible(false);
}
void ContextMenu::draw(Block& block)
{
	m_root.m_panel->setVisible(true);
	m_root.m_panel->setOrigin(1, getOriginYForMousePosition());
	auto xPosition = std::max(ContextMenuSegment::width, sf::Mouse::getPosition().x);
	m_root.m_panel->setPosition(xPosition, sf::Mouse::getPosition().y);
	m_root.m_grid->removeAllWidgets();
	m_submenus.clear();
	auto blockInfoButton = tgui::Button::create("location info");
	m_root.add(blockInfoButton);
	blockInfoButton->onClick([this, &block]{ m_window.getGameOverlay().drawInfoPopup(block); });
	//TODO: shift to add to end of work queue.
	if(block.isSolid())
		drawDigControls(block);
	else // Clicked on block is not solid.
	{
		drawConstructControls(block);
		drawActorControls(block);
		drawPlantControls(block);
		drawItemControls(block);
		drawFarmFieldControls(block);
		drawStockPileControls(block);
		drawCraftControls(block);
		drawWoodCuttingControls(block);
		if(m_window.m_editMode)
		{
			drawFluidControls(block);
			auto factionsButton = tgui::Button::create("factions");
			m_root.add(factionsButton);
			factionsButton->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			factionsButton->onClick([this]{ m_window.showEditFactions(); });
		}
	}
}
ContextMenuSegment& ContextMenu::makeSubmenu(size_t index)
{
	assert(m_submenus.size() + 1 > index);
	// destroy old submenus.
	m_submenus.resize(index, {m_gameOverlayGroup});
	m_submenus.emplace_back(m_gameOverlayGroup);
	auto& submenu = m_submenus.back();
	m_gameOverlayGroup->add(submenu.m_panel);
	ContextMenuSegment& previous = index == 0 ? m_root : m_submenus.at(index - 1);
	sf::Vector2i pixelPos = sf::Mouse::getPosition();
	submenu.m_panel->setOrigin(0, getOriginYForMousePosition());
	submenu.m_panel->setPosition(tgui::bindRight(previous.m_panel), pixelPos.y);
	return submenu;
}
void ContextMenu::hide()
{
	m_window.getGameOverlay().unfocusUI();
	m_submenus.clear();
	m_root.m_grid->removeAllWidgets();
	m_root.m_panel->setVisible(false);
}
[[nodiscard]] float ContextMenu::getOriginYForMousePosition() const
{
	sf::Vector2i pixelPos = sf::Mouse::getPosition();
	int windowHeight = m_window.getRenderWindow().getSize().y;
	if(pixelPos.y < ((int)m_window.getRenderWindow().getSize().y / 4))
		return 0;
	else if(pixelPos.y > (windowHeight * 3) / 4)
		return 1;
	return 0.5;
}
