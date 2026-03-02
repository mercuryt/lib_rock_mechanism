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
#include "../engine/geometry/cuboidSet.h"
#include "../engine/reference.h"
#include "contextMenu/contextMenu.h"
class Window;
class Uniform;
enum class SelectMode{Space, Actors, Items, Plants};
enum class InfoPopUpId { Point, Actor, Item, Plant, Null };
class GameOverlay final
{
public:
	SDL_Point m_mouseDragStart;
	CuboidSet m_selectedArea;
	SmallSet<ActorReference> m_selectedActors;
	SmallSet<ItemReference> m_selectedItems;
	// No set for plants, instead use selectedArea;
	CuboidSet m_selectedSpace;
	Uniform* m_uniformToEdit;
	Point3D m_blockUnderCursor;
	ActorReference m_detailActor;
	ItemReference m_detailItem;
	ItemReference m_itemBeingInstalled;
	// TODO: Change to shape being moved for targeted hauling of downed actors.
	ItemReference m_itemBeingMoved;
	Point3D m_detailPoint;
	Facing4 m_facing;
	InfoPopUpId m_infoPopUp = InfoPopUpId::Null;
	SelectMode m_selectMode = SelectMode::Space;
	ContextMenuState m_contextMenuState;
	FactionId m_factionToEdit;
	bool m_mouseIsDown = false;
	bool m_gameMenuIsOpen = false;
	void draw(Window& window);
	void drawSelectionBox(Window& window);
	void drawTopBar(Window& window);
	void drawMenu(Window& window);
	void drawSelection(Window& window);
	void deselectAll();
	void updateSelect(Window& window, const Cuboid cuboid);
	void drawInfoPopUp(Window& window);
	void showInfoPopUpForActor(const ActorReference actor);
	void showInfoPopUpForItem(const ItemReference item);
	void showInfoPopUpForPoint(const Point3D point);
	// Plant is referenced by location.
	void showInfoPopUpPlant(const Point3D point);
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
