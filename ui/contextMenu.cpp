#include "contextMenu.h"
#include "../engine/pointFeature.h"
#include "../engine/config/config.h"
#include "../engine/designations.h"
#include "displayData.h"
#include "../engine/plants.h"
#include "../engine/space/space.h"
#include "widgets.h"
#include "window.h"
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
void ContextMenu::draw(const Point3D& point)
{
	m_root.m_panel->setVisible(true);
	m_root.m_panel->setOrigin(1, getOriginYForMousePosition());
	auto xPosition = std::max(ContextMenuSegment::width, sf::Mouse::getPosition().x);
	m_root.m_panel->setPosition(xPosition, sf::Mouse::getPosition().y);
	m_root.m_grid->removeAllWidgets();
	m_submenus.clear();
	auto blockInfoButton = tgui::Button::create("location info");
	m_root.add(blockInfoButton);
	blockInfoButton->onClick([this, point]{ m_window.getGameOverlay().drawInfoPopup(point); });
	Space& space = m_window.getArea()->getSpace();
	//TODO: shift to add to end of work queue.
	if(space.solid_isAny(point) || !space.pointFeature_empty(point))
		drawDigControls(point);
	if(!space.solid_isAny(point))
	{
		drawConstructControls(point);
		drawActorControls(point);
		drawPlantControls(point);
		drawItemControls(point);
		drawFarmFieldControls(point);
		drawStockPileControls(point);
		drawCraftControls(point);
		drawWoodCuttingControls(point);
	}
	if(m_window.m_editMode)
	{
		drawFluidControls(point);
		auto factionsButton = tgui::Button::create("factions");
		m_root.add(factionsButton);
		factionsButton->onClick([this]{
			m_window.showEditFactions();
			hide();
		});
		auto dramaButton = tgui::Button::create("drama");
		m_root.add(dramaButton);
		dramaButton->onClick([this, point]{
			m_window.showEditDrama();
			hide();
		});
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
