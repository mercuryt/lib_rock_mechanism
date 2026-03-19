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

struct ControllsState final
{
	Point3D clickedOnPoint;
	MaterialTypeId materialType;
	PointFeatureTypeId featureType = PointFeatureTypeId::Ramp;
	FluidTypeId fluidType;
	int fluidVolume = Config::maxPointVolume.get();
	bool constructed = true;
	Percent percentGrown{100};
	PlantSpeciesId plantSpecies;
	AnimalSpeciesId animalSpecies;
	ItemTypeId itemType;
	FactionId faction;
	Percent wear{0};
	Quality quality{0};
	Quantity quantity{1};
	Facing4 facing = Facing4::North;
	bool installed = false;
	std::string name;
	void initalize();
};

class GameOverlay final
{
public:
	// Coordinates as float in 2d for drawing selection box, point is 3d for creating selected cuboids.
	SDL_Point m_mouseDragStartCoordinates;
	Point3D m_mouseDragStartPoint;
	CuboidSet m_selectedArea;
	SmallSet<ActorReference> m_selectedActors;
	SmallSet<ItemReference> m_selectedItems;
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
	ControllsState m_controllsState;
	FactionId m_factionToEdit;
	bool m_mouseIsDown = false;
	bool m_gameMenuIsOpen = false;
	void draw(Window& window);
	void drawSelectionBox(Window& window);
	void drawTopBar(Window& window);
	void drawMenu(Window& window);
	void deselectAll();
	void updateSelect(Window& window, const Cuboid cuboid);
	void drawInfoPopUp(Window& window);
	void showInfoPopUpForActor(const ActorReference actor);
	void showInfoPopUpForItem(const ItemReference item);
	void showInfoPopUpForPoint(const Point3D point);
	// Plant is referenced by location.
	void showInfoPopUpPlant(const Point3D point);
};
