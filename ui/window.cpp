#include "window.h"
#include "../engine/config/config.h"
#include "craft.h"
#include "displayData.h"
#include "sprite.h"
#include "../engine/simulation/hasAreas.h"
#include "../engine/space/space.h"
#include "../engine/items/items.h"
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <TGUI/Widgets/FileDialog.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <ratio>
#include <mutex>
#include <string>
#include <unordered_set>
#include <fstream>
Window::Window() : m_window(sf::VideoMode::getDesktopMode(), "Goblin Pit", sf::Style::Fullscreen), m_gui(m_window), m_view(m_window.getDefaultView()),
	m_scaledView(m_view),
	m_simulation(nullptr), m_mainMenuView(*this), m_loadView(*this), m_gameOverlay(*this), m_dialogueBoxUI(*this), m_objectivePriorityView(*this),
	m_productionView(*this), m_uniformView(*this), m_stocksView(*this), m_actorView(*this), //m_worldParamatersView(*this),
	m_editRealityView(*this), m_editActorView(*this), m_editAreaView(*this), m_editFactionView(*this), m_editFactionsView(*this),
	m_editStockPileView(*this), m_editDramaView(*this), m_backgroundTask(*this), m_minimumTimePerFrame(200),
	m_minimumTimePerStep((int)(1000.f / (float)Config::stepsPerSecond.get())), m_draw(*this),
	m_simulationThread([&](){
		while(true)
		{
			std::chrono::milliseconds start;
			start = msSinceEpoch();
			if(m_simulation && !m_paused)
				m_simulation->doStep(m_speed);
			std::chrono::milliseconds delta = msSinceEpoch() - start;
			if(delta < m_minimumTimePerStep)
				std::this_thread::sleep_for(m_minimumTimePerStep - delta);
		}
	}), m_zoom(1.f), m_editMode(false)
{
	setPaused(true);
	m_font.loadFromFile("lib/fonts/UbuntuMono-R.ttf");
	//m_font.loadFromFile("lib/fonts/NotoEmoji-Regular.ttf");
	showMainMenu();
}
void Window::setArea(Area& area, GameView* gameView)
{
	if(!gameView)
	{
		if(m_lastViewedSpotInArea.contains(area.m_id))
			gameView = &m_lastViewedSpotInArea[area.m_id];
		else
			gameView = &m_lastViewedSpotInArea.emplace(area.m_id,  area.getSpace().getCenterAtGroundLevel(), displayData::defaultScale);
	}
	m_area = &area;
	centerView(gameView->center);
}
void Window::centerView(const Point3D& point)
{
	m_z = point.z();
	sf::Vector2f globalPosition(point.x().get() * displayData::defaultScale, point.y().get() * displayData::defaultScale);
	sf::Vector2i pixelPosition = m_window.mapCoordsToPixel(globalPosition);
	m_scaledView.setCenter(pixelPosition.x, pixelPosition.y);
}
void Window::hideAllPanels()
{
	m_mainMenuView.hide();
	m_gameOverlay.hide();
	m_loadView.close();
	m_objectivePriorityView.hide();
	m_productionView.hide();
	m_uniformView.hide();
	m_editAreaView.hide();
	m_editRealityView.hide();
	m_actorView.hide();
	m_editActorView.hide();
	m_editFactionView.hide();
	m_editFactionsView.hide();
	m_editStockPileView.hide();
	m_editDramaView.hide();
	//m_worldParamatersView.hide();
}
void Window::startLoop()
{
	while (m_window.isOpen())
	{
		std::chrono::milliseconds start = msSinceEpoch();
		sf::Event event;
		while (m_window.pollEvent(event))
		{
			if(m_gui.handleEvent(event))
				continue;
			//TODO: check if modifiers were pressed at the time the event was generated, not now.
			int32_t scrollSteps = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ? 6 : 1;
			if(m_area)
				m_blockUnderCursor = getBlockUnderCursor();
			switch(event.type)
			{
				case sf::Event::Closed:
					m_window.close();
					break;
				case sf::Event::KeyPressed:
					switch(event.key.code)
					{
						case sf::Keyboard::Q:
							if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
								m_window.close();
							break;
						case sf::Keyboard::S:
							if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
								save();
							break;
						case sf::Keyboard::LAlt:
							if(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
							{
								if(m_selectMode == SelectMode::Space)
									m_selectMode = SelectMode::Actors;
								else
									m_selectMode = SelectMode((int)m_selectMode + 1);
							}
							else
							{
								if(m_selectMode == SelectMode::Actors)
									m_selectMode = SelectMode::Space;
								else
									m_selectMode = SelectMode((int)m_selectMode - 1);
							}
							break;
						case sf::Keyboard::PageUp:
							if(m_area != nullptr && m_gameOverlay.isVisible() && (m_z + 1) < m_area->getSpace().m_sizeZ)
								m_z += 1;
							break;
						case sf::Keyboard::PageDown:
							if(m_gameOverlay.isVisible() && m_z > 0)
								m_z -= 1;
							break;
						case sf::Keyboard::Up:
							if(m_gameOverlay.isVisible() && m_scaledView.getCenter().y > (m_scaledView.getSize().y / 2.f) - gameMarginSize)
								m_scaledView.move(0.f, scrollSteps * -20.f);
							break;
						case sf::Keyboard::Down:
							if(m_area != nullptr && m_gameOverlay.isVisible() && m_scaledView.getCenter().y < m_area->getSpace().m_sizeY.get() * displayData::defaultScale - (m_scaledView.getSize().y / 2.f) + gameMarginSize)
								m_scaledView.move(0.f, scrollSteps * 20.f);
							break;
						case sf::Keyboard::Left:
							if(m_gameOverlay.isVisible() && m_scaledView.getCenter().x > (m_scaledView.getSize().x / 2.f) - gameMarginSize)
								m_scaledView.move(scrollSteps * -20.f, 0.f);
							break;
						case sf::Keyboard::Right:
							if(m_area != nullptr && m_gameOverlay.isVisible() && m_scaledView.getCenter().x < m_area->getSpace().m_sizeX.get() * displayData::defaultScale - (m_scaledView.getSize().x / 2.f) + gameMarginSize)
								m_scaledView.move(scrollSteps * 20.f, 0.f);
							break;
						case sf::Keyboard::Delete:
								// TODO: detect passing through zoom level 1 and reset view to default to zero out errors.
								m_zoom -= displayData::zoomIncrement;
								m_scaledView.zoom(1.f + displayData::zoomIncrement);
							break;
						case sf::Keyboard::Insert:
								// TODO: detect passing through zoom level 1 and reset view to default to zero out errors.
								m_zoom += displayData::zoomIncrement;
								m_scaledView.zoom(1.f - displayData::zoomIncrement);
							break;
						case sf::Keyboard::Period:
							if(m_gameOverlay.isVisible() && m_paused)
								m_simulation->doStep();
							break;
						case sf::Keyboard::Space:
							if(m_gameOverlay.isVisible() && !m_gameOverlay.menuIsVisible())
								setPaused(!m_paused);
							break;
						case sf::Keyboard::Escape:
							if(m_gameOverlay.isVisible())
							{
								if(m_gameOverlay.menuIsVisible())
									m_gameOverlay.closeMenu();
								else if(m_gameOverlay.contextMenuIsVisible())
									m_gameOverlay.closeContextMenu();
								else if(m_gameOverlay.infoPopupIsVisible())
									m_gameOverlay.closeInfoPopup();
								else if(m_gameOverlay.m_itemBeingInstalled.exists())
									m_gameOverlay.m_itemBeingInstalled.clear();
								else if(m_gameOverlay.m_itemBeingMoved.exists())
									m_gameOverlay.m_itemBeingMoved.clear();
								else
									m_gameOverlay.drawMenu();
							}
							else if(m_productionView.isVisible())
								if(m_productionView.createIsVisible())
									m_productionView.closeCreate();
								else
									showGame();
							else
							{
								showGame();
							}
							break;
						case sf::Keyboard::Num1:
							if(m_area)
								showGame();
							break;
						case sf::Keyboard::Num2:
							if(m_area)
								showStocks();
							break;
						case sf::Keyboard::Num3:
							if(m_area)
								showProduction();
							break;
						case sf::Keyboard::Num4:
							if(m_area)
								showUniforms();
							break;
						case sf::Keyboard::Num5:
							if(m_area && m_selectedActors.size() == 1)
								showActorDetail(*m_selectedActors.begin());
							break;
						case sf::Keyboard::Num6:
							if(m_area && m_selectedActors.size() == 1)
								showObjectivePriority(*m_selectedActors.begin());
							break;
						case sf::Keyboard::F1:
							if(m_area)
								setSpeed(1);
							break;
						case sf::Keyboard::F2:
							if(m_area)
								setSpeed(4);
							break;
						case sf::Keyboard::F3:
							if(m_area)
								setSpeed(8);
							break;
						case sf::Keyboard::F4:
							if(m_area)
								setSpeed(16);
							break;
						case sf::Keyboard::F5:
							if(m_area)
								setSpeed(64);
							break;
						case sf::Keyboard::F6:
							if(m_area)
								setSpeed(128);
							break;
						case sf::Keyboard::F7:
							if(m_area)
								setSpeed(256);
							break;
						case sf::Keyboard::F8:
							if(m_area)
								setSpeed(512);
							break;
						case sf::Keyboard::F9:
							if(m_area)
								setSpeed(1024);
							break;
						default:
							break;
					}
					break;
				case sf::Event::MouseButtonPressed:
				{
					if(m_area)
					{
						if(event.mouseButton.button == displayData::selectMouseButton)
						{
							m_gameOverlay.closeContextMenu();
							m_firstCornerOfSelection = getBlockAtPosition({event.mouseButton.x, event.mouseButton.y});
							m_positionWhereMouseDragBegan = {event.mouseButton.x, event.mouseButton.y};
							if(!sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
								deselectAll();
						}
					}
					break;
				}
				case sf::Event::MouseButtonReleased:
				{
					if(m_area)
					{
						Space& space = m_area->getSpace();
						Items& items = m_area->getItems();
						Point3D& point = m_blockUnderCursor;
						if(event.mouseButton.button == displayData::selectMouseButton)
						{
							Cuboid selectedBlocks;
							// Find the selected area.
							if(m_firstCornerOfSelection.exists())
								selectedBlocks.setFrom(m_firstCornerOfSelection, point);
							else
								selectedBlocks.setFrom(point);
							m_firstCornerOfSelection.clear();
							bool deselect = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl);
							switch(m_selectMode)
							{
								case(SelectMode::Actors):
									if(deselect)
										for(const Point3D& selectedPoint : selectedBlocks)
											m_selectedActors.maybeEraseAll(space.actor_getAll(selectedPoint));
									else
										for(const Point3D& selectedPoint : selectedBlocks)
											m_selectedActors.maybeInsertAll(space.actor_getAll(selectedPoint));
									break;
								case(SelectMode::Items):
									if(deselect)
										for(const Point3D& selectedPoint : selectedBlocks)
											m_selectedItems.maybeEraseAll(space.item_getAll(selectedPoint));
									else
										for(const Point3D& selectedPoint : selectedBlocks)
											m_selectedItems.maybeInsertAll(space.item_getAll(selectedPoint));
									break;
								case(SelectMode::Plants):
									if(deselect)
										for(const Point3D& selectedPoint : selectedBlocks)
											m_selectedPlants.maybeErase(space.plant_get(selectedPoint));
									else
										for(const Point3D& selectedPoint : selectedBlocks)
										{
											const PlantIndex& plant = space.plant_get(selectedPoint);
											if(plant.exists())
												m_selectedPlants.maybeInsert(plant);
										}
									break;
								case(SelectMode::Space):
									if(deselect)
										m_selectedBlocks.remove(selectedBlocks);
									else
										m_selectedBlocks.add(selectedBlocks);
									break;
							}
						}
						else if(event.mouseButton.button == displayData::actionMouseButton)
						{
							// Display context menu.
							//TODO: Left click and drag shortcut for select and open context.
							const ItemIndex& item = m_gameOverlay.m_itemBeingInstalled;
							if(
									item.exists() &&
									space.shape_shapeAndMoveTypeCanEnterEverWithFacing(point, items.getShape(item), items.getMoveType(item), m_gameOverlay.m_facing)
							)
								m_gameOverlay.assignLocationToInstallItem(point);
							else if(m_gameOverlay.m_itemBeingMoved.exists())
							{
								if(getSelectedActors().empty())
									m_gameOverlay.m_itemBeingMoved.clear();
								else if(space.shape_shapeAndMoveTypeCanEnterEverWithFacing(point, items.getShape(item), items.getMoveType(item), m_gameOverlay.m_facing))
									m_gameOverlay.assignLocationToMoveItemTo(point);
							}
							else
								m_gameOverlay.drawContextMenu(point);
						}
						else
							m_gameOverlay.closeContextMenu();
					}
					break;
				}
				default:
					if(m_area)
					{
						m_gameOverlay.m_coordinateUI->setText(
							std::to_string(m_blockUnderCursor.x().get()) + "," +
							std::to_string(m_blockUnderCursor.y().get()) + "," +
							std::to_string(m_blockUnderCursor.z().get())
						);
					}
					break;
			}
		}
		// Begin draw.
		m_window.clear();
		// Draw main m_scaledView.
		if(m_simulation && m_area)
		{
			// Draw dialogueBox.
			if(!m_dialogueBoxUI.empty() && !m_dialogueBoxUI.isVisible())
			{
				m_paused = true;
				m_dialogueBoxUI.draw();
			}
			if(!m_paused && m_gameOverlay.infoPopupIsVisible())
				m_gameOverlay.updateInfoPopup();
			// Set window view to scaled view before drawing.
			m_window.setView(m_scaledView);
			m_draw.view();
		}
		// Change window to default view before drawing GUI to prevent it being scaled or moved.
		m_window.setView(m_view);
		// Draw UI.
		m_gui.draw();
		// Finalize draw.
		m_window.display();
		// Check if background task is done.
		m_backgroundTask.onFrame();
		// Frame rate limit.
		std::chrono::milliseconds delta = msSinceEpoch() - start;
		std::chrono::milliseconds minimum = m_speed < 200 ? m_minimumTimePerFrame : m_minimumTimePerFrame * 3;
		if(minimum > delta)
			std::this_thread::sleep_for(minimum - delta);
	}
}
void Window::threadTask(std::function<void()>&& task, std::function<void()>&& callback)
{
	m_backgroundTask.create(std::move(task), std::move(callback));
}
void Window::save()
{
	assert(m_simulation);
	std::function<void()> task = [this]{
		m_simulation->save();
		if(getArea() != nullptr)
		{
			const Json povData = povToJson();
			std::ofstream file(m_simulation->getPath()/"pov.json" );
			file << povData;
		}
	};
	threadTask(std::move(task));
}
void Window::load(std::filesystem::path path)
{
	deselectAll();
	Json povData;
	bool povExists = false;
	std::function<void()> task = [this, povData, povExists, path] mutable {
		m_simulation = std::make_unique<Simulation>(path);
		std::filesystem::path viewPath = m_simulation->getPath()/"pov.json";
		if(std::filesystem::exists(viewPath))
		{
			std::ifstream af(viewPath);
			povData = Json::parse(af);
			povExists = true;
		}
	};
	std::function<void()> callback = [&]
	{
		if(povExists)
		{
			povFromJson(povData);
			assert(m_area != nullptr);
			showGame();
		}
		else
			showEditReality();
	};
	threadTask(std::move(task), std::move(callback));
}
void Window::setPaused(const bool paused)
{
	m_paused = paused;
	setSpeedDisplay();
}
void Window::togglePaused()
{
	// Atomic toggle. probably not needed.
	m_paused.toggle();
	setSpeedDisplay();
}
void Window::setSpeed(int16_t speed)
{
	m_speed = speed;
	setSpeedDisplay();
}
void Window::setSpeedDisplay()
{
	std::string display = m_paused ? "paused" : "speed: " + (m_speed ? std::to_string(m_speed) : "max");
	m_gameOverlay.m_speedUI->setText(display);
}
void Window::povFromJson(const Json& data)
{
	assert(m_simulation);
	if(data.contains("faction"))
		m_faction = data["faction"].get<FactionId>();
	m_area = &m_simulation->m_hasAreas->getById(data["area"].get<AreaId>());
	m_z = Distance::create(data["z"].get<int32_t>());
	int32_t x = data["x"].get<int32_t>();
	int32_t y = data["y"].get<int32_t>();
	m_scaledView.setCenter(x, y);
}
void Window::deselectAll()
{
	m_selectedBlocks.clear();
	m_selectedItems.clear();
	m_selectedActors.clear();
	m_selectedPlants.clear();
}
void Window::selectBlock(const Point3D& point)
{
	assert(m_area != nullptr);
	deselectAll();
	m_selectedBlocks.add(point);
}
void Window::selectItem(const ItemIndex& item)
{
	deselectAll();
	m_selectedItems.insert(item);
}
void Window::selectPlant(const PlantIndex& plant)
{
	deselectAll();
	m_selectedPlants.insert(plant);
}
void Window::selectActor(const ActorIndex& actor)
{
	deselectAll();
	m_selectedActors.insert(actor);
}
Point3D Window::getBlockUnderCursor()
{
	sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
	return getBlockAtPosition(pixelPos);
}
Point3D Window::getBlockAtPosition(sf::Vector2i pixelPos)
{
	sf::Vector2f worldPos = m_window.mapPixelToCoords(pixelPos, m_scaledView);
    // Convert world coordinates to block coordinates
    int32_t x = static_cast<int32_t>(worldPos.x / displayData::defaultScale);
    int32_t y = static_cast<int32_t>(worldPos.y / displayData::defaultScale);
    Space& space = m_area->getSpace();
    x = std::clamp(x, 0, space.m_sizeX.get() - 1);
    y = std::clamp(y, 0, space.m_sizeY.get() - 1);
    return Point3D::create(x, invertY(y), m_z.get());
}
void Window::setFaction(const FactionId& faction)
{
	m_faction = faction;
}
[[nodiscard]] Json Window::povToJson() const
{
	Json data{
		{"area", m_area},
		{"z", m_z},
		{"x", m_scaledView.getCenter().x},
		{"y", m_scaledView.getCenter().y}
	};
	if(m_faction.exists())
		data["faction"] = m_faction;
	return data;
}
// static method
std::chrono::milliseconds Window::msSinceEpoch()
{
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}
std::string Window::displayNameForItem(const ItemIndex& item)
{
	Items& items = m_area->getItems();
	std::string output = items.getName(item);
	if(!output.empty())
		output.append(" a ");
	output.append(MaterialType::getName(items.getMaterialType(item)));
		output.append(" ");
	output.append(ItemType::getName(items.getItemType(item)));
	return output;
}
std::string Window::displayNameForCraftJob(CraftJob& craftJob)
{
	std::string output;
	output.append(CraftJobType::getName(craftJob.craftJobType));
	if(craftJob.materialType.exists())
	{
		output.append(" ");
		output.append(MaterialType::getName(craftJob.materialType));
	}
	return output;
}
std::string Window::facingToString(Facing4 facing)
{
	if(facing == Facing4::North)
		return "up";
	if(facing == Facing4::East)
		return "right";
	if(facing == Facing4::South)
		return "down";
	assert(facing == Facing4::West);
	return "left";
}
int Window::invertY(const int& distance) const
{
	assert(m_area != nullptr);
	return (m_area->getSpace().m_sizeY.get() - 1) - distance;
}
void Window::setCursor(const sf::Cursor::Type& newCursor)
{
	sf::Cursor cursor;
	if (cursor.loadFromSystem(newCursor))
		m_window.setMouseCursor(cursor);
}