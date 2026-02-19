#pragma once

#include <SDL2/SDL.h>
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wdouble-promotion\"")
#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
_Pragma("GCC diagnostic pop")
//#include "contextMenu.h"
//#include "infoPopup.h"
#include "../engine/numericTypes/types.h"
#include "../engine/numericTypes/index.h"
class Window;
class GameOverlay final
{
public:
	ItemIndex m_itemBeingInstalled;
	// TODO: Change to shape being moved for targeted hauling of downed actors.
	ItemIndex m_itemBeingMoved;
	Facing4 m_facing;
	SDL_Point m_mouseDragStart;
	bool m_mouseIsDown = false;
	bool m_gameMenuIsOpen = false;
	void draw(Window& window);
	void drawSelectionBox(Window& window);
	void drawTopBar(Window& window);
	void drawMenu(Window& window);

	/*
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void drawContextMenu(const Point3D& point) { m_contextMenu.draw(point); }
	void closeContextMenu() { m_contextMenu.hide(); }
	void drawMenu();
	void closeMenu() { m_menu->setVisible(false); }
	void drawInfoPopup(const Point3D& point) { m_infoPopup.display(point); }
	void drawInfoPopup(const ItemIndex& item) { m_infoPopup.display(item); }
	void drawInfoPopup(const PlantIndex& plant) { m_infoPopup.display(plant); }
	void drawInfoPopup(const ActorIndex& actor) { m_infoPopup.display(actor); }
	void closeInfoPopup() { m_infoPopup.hide(); };
	void assignLocationToInstallItem(const Point3D& point);
	void assignLocationToMoveItemTo(const Point3D& point);
	void unfocusUI();
	void drawTime();
	void drawZoom();
	void drawWeatherReport();
	void updateInfoPopup() { m_infoPopup.update(); }
	void drawSelectionDescription();
	[[nodiscard]] bool isVisible() const { return m_group->isVisible(); }
	[[nodiscard]] bool menuIsVisible() const { return m_menu->isVisible(); }
	[[nodiscard]] bool contextMenuIsVisible() const { return m_contextMenu.isVisible(); }
	[[nodiscard]] bool infoPopupIsVisible() const { return m_infoPopup.isVisible(); }
	[[nodiscard]] tgui::Group::Ptr getGroup() const { return m_group; }
	*/
};
