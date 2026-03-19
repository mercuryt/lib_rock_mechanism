#include "gameOverlay.h"
#include "window.h"
#include "displayData.h"
#include "draw.h"
#include "infoPopUp.h"
#include "../engine/area/area.h"
#include "../engine/space/space.h"
#include "../engine/actors/actors.h"
#include "../engine/items/items.h"
#include "../engine/plants.h"
#include "../engine/definitions/plantSpecies.h"
#include "../engine/definitions/animalSpecies.h"
void GameOverlay::draw(Window& window)
{
	drawSelectionBox(window);
	drawTopBar(window);
	drawInfoPopUp(window);
	contextMenu::draw(window);
	if(m_gameMenuIsOpen)
		drawMenu(window);
}
void GameOverlay::drawSelectionBox(Window& window)
{
	if(!window.m_gameOverlay.m_mouseIsDown)
		return;
	SDL_Rect rect;
	rect.x = std::min(m_mouseDragStartCoordinates.x, window.m_mousePosition.x);
	rect.y = std::min(m_mouseDragStartCoordinates.y, window.m_mousePosition.y);
	rect.w = std::abs(window.m_mousePosition.x - m_mouseDragStartCoordinates.x);
	rect.h = std::abs(window.m_mousePosition.y - m_mouseDragStartCoordinates.y);
	if(rect.w > displayData::selectBoxThickness * 2 && rect.h > displayData::selectBoxThickness * 2)
	{
		SDL_Color color = window.m_controllKey? displayData::cancelColor : displayData::selectColor;
		SDL_SetRenderDrawColor(window.m_sdlRenderer, color.r, color.g, color.b, color.a);
		// This issues an extra draw call but is kept seperate from the RenderBuffer because it is not scaled or panned.
		SDL_RenderDrawRect(window.m_sdlRenderer, &rect);
		SDL_SetRenderDrawColor(window.m_sdlRenderer, 255, 255, 255, 255);
	}
}
void GameOverlay::drawTopBar(Window& window)
{
	ImGuiIO& io = ImGui::GetIO();
	float windowWidth = io.DisplaySize.x;
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::SetNextWindowSize(ImVec2(windowWidth, 0.f));
	ImGui::Begin("topBar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs
	);
	const DateTime& now = window.m_simulation->getDateTime();
	// TODO: getting seconds and minutes from hourlyEvent.percentComplete is roundabout and wierd, add them to DateTime instead?
	const float fractionOfCurrentHourElapsed = window.m_simulation->m_hourlyEvent.fractionComplete();
	int seconds = Config::minutesPerHour * Config::secondsPerMinute * fractionOfCurrentHourElapsed;
	const int minutes = seconds / Config::secondsPerMinute;
	seconds -= minutes * Config::secondsPerMinute;
	ImGui::Text(
		"seconds: %i minutes: %i hour: %i day: %i year: %i",
		seconds, minutes, now.hour, now.day, now.year
	);
	ImGui::SameLine();
	const float yPos = ImGui::GetItemRectMin().y;
	ImGui::SetCursorScreenPos({windowWidth / 3.f, yPos});
	ImGui::Text(window.m_paused ? "paused" : "speed: %.2f", window.m_speed.load());
	ImGui::SameLine();
	ImGui::SetCursorScreenPos({windowWidth * 6 / 10.f, yPos});
	ImGui::Text("%i, %i, %i", m_blockUnderCursor.x().get(), m_blockUnderCursor.y().get(), m_blockUnderCursor.z().get());
	ImGui::SameLine();
	ImGui::SetCursorScreenPos({windowWidth * 8 / 10.f, yPos});
	std::string selectModeName;
	int selectedCount;
	switch(m_selectMode)
	{
		case SelectMode::Space:
			selectModeName = "space";
			selectedCount = m_selectedArea.volume();
			break;
		case SelectMode::Actors:
			selectModeName = "actors";
			selectedCount = m_selectedActors.size();
			break;
		case SelectMode::Items:
			selectModeName = "items";
			selectedCount = m_selectedItems.size();
			break;
		case SelectMode::Plants:
			selectModeName = "plants";
			selectedCount = window.m_area->getSpace().plant_count(m_selectedArea);
			break;
	}
	ImGui::Text("selecting: %s, selected : %i", selectModeName.c_str(), selectedCount);

	// TODO: weather.
	ImGui::End();
}
void GameOverlay::drawMenu(Window& window)
{
	ImGui::PushFont(nullptr, displayData::menuFontSize);
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	// Set window size and position in logical space.
	ImVec2 windowSize = ImVec2(400, 400);
	ImVec2 centerPos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
	// Center window using pivot (0.5,0.5)
	ImGui::SetNextWindowSize(windowSize);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::Begin("overlayMenu", &canClose, window.m_menuWindowFlags);
	auto centerButton = [&](const char label[], auto callback)
	{
			float buttonWidth = ImGui::CalcTextSize(label).x + 20; // add padding
			ImGui::SetCursorPosX((windowSize.x - buttonWidth) * 0.5f);
			if(ImGui::Button(label, ImVec2(buttonWidth, 0)))
					callback();
	};
	centerButton("Menu", [&]{ m_gameMenuIsOpen = false; window.showMainMenu(); });
	ImGui::BeginDisabled(window.m_backgroundTask.running());
	centerButton("Save", [&]{ window.save(); });
	auto quitContinuation = [&]{ window.save([windowPtr = &window]{ windowPtr->quit(); }); };
	centerButton("Save And Quit", std::move(quitContinuation));
	centerButton("Close", [&]{ m_gameMenuIsOpen = false; });
	ImGui::EndDisabled();
	ImGui::PopFont();
	ImGui::End();
}
void GameOverlay::deselectAll()
{
	m_selectedArea.clear();
	m_selectedActors.clear();
	m_selectedItems.clear();
}
void GameOverlay::updateSelect(Window& window, const Cuboid cuboid)
{
	Space& space = window.m_area->getSpace();
	CuboidSet revealedPartOfSelection = CuboidSet::create(cuboid);
	space.unrevealed_queryForEach(cuboid, [&](const Cuboid unrevealedCuboid){ revealedPartOfSelection.remove(unrevealedCuboid); });
	switch(m_selectMode)
	{
		case SelectMode::Space:
			{
				if(window.m_controllKey)
					m_selectedArea.maybeRemove(cuboid);
				else
				{
					if(!window.m_shiftKey)
						m_selectedArea.clear();
					// TODO: Only being able to select revealed would be very anoying for digging out exploritory shafts. Create a m_provisionalDig rtree which is added to when unrevealed are selected for digging and which is removed from when revealed, possibly creating a dig designation.
					m_selectedArea.maybeAddAll(revealedPartOfSelection);
				}
			}
			break;
		case SelectMode::Actors:
			{
				SmallSet<ActorReference> actors = window.m_area->getSpace().actor_getAllReferences(window.m_area->getActors(), revealedPartOfSelection);
				if(window.m_controllKey)
					m_selectedActors.maybeEraseAll(actors);
				else
				{
					if(!window.m_shiftKey)
						m_selectedActors.clear();
					m_selectedActors.maybeInsertAll(actors);
				}
			}
			break;
		case SelectMode::Items:
			{
				SmallSet<ItemReference> items = window.m_area->getSpace().item_getAllReferences(window.m_area->getItems(), revealedPartOfSelection);
				if(window.m_controllKey)
					m_selectedItems.eraseAll(items);
				else
				{
					if(!window.m_shiftKey)
						m_selectedItems.clear();
					m_selectedItems.maybeInsertAll(items);
				}
			}
			break;
		case SelectMode::Plants:
			{
				CuboidSet areaContainingPlants;
				window.m_area->getSpace().plant_queryForEachCuboid(revealedPartOfSelection, [&](const Cuboid& occupied){
					areaContainingPlants.maybeAdd(occupied);
				});
				if(window.m_controllKey)
					m_selectedArea.maybeRemoveAll(areaContainingPlants);
				else
				{
					if(!window.m_shiftKey)
						m_selectedArea.clear();
					m_selectedArea.maybeAddAll(areaContainingPlants);
				}
			}
			break;
	}
}
void GameOverlay::drawInfoPopUp(Window& window)
{
	switch(m_infoPopUp)
	{
		case InfoPopUpId::Actor:
			drawInfoPopUp::actor(window);
			break;
		case InfoPopUpId::Item:
			drawInfoPopUp::item(window);
			break;
		case InfoPopUpId::Point:
			drawInfoPopUp::point(window);
			break;
		case InfoPopUpId::Plant:
			drawInfoPopUp::plant(window);
			break;
		default:
			std::unreachable();
	}
}
void GameOverlay::showInfoPopUpForActor(const ActorReference actor)
{
	m_infoPopUp = InfoPopUpId::Actor;
	m_detailActor = actor;
}
void GameOverlay::showInfoPopUpForItem(const ItemReference item)
{
	m_infoPopUp = InfoPopUpId::Item;
	m_detailItem = item;
}
void GameOverlay::showInfoPopUpForPoint(const Point3D point)
{
	m_infoPopUp = InfoPopUpId::Point;
	m_detailPoint = point;
}
void GameOverlay::showInfoPopUpPlant(const Point3D point)
{
	m_infoPopUp = InfoPopUpId::Plant;
	m_detailPoint = point;
}
void ControllsState::initalize()
{
	materialType = MaterialType::byName("dirt");
	fluidType = FluidType::byName("water");
	plantSpecies = PlantSpecies::byName("wheat grass");
	animalSpecies = AnimalSpecies::byName("goblin");
	itemType = ItemType::byName("pile");
}