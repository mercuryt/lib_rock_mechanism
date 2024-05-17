#include "window.h"
#include "config.h"
#include "craft.h"
#include "displayData.h"
#include "sprite.h"
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
Window::Window() : m_window(sf::VideoMode::getDesktopMode(), "Goblin Pit", sf::Style::Fullscreen), m_gui(m_window), m_view(m_window.getDefaultView()), 
	m_mainMenuView(*this), m_loadView(*this), m_gameOverlay(*this), m_dialogueBoxUI(*this), m_objectivePriorityView(*this), 
	m_productionView(*this), m_uniformView(*this), m_stocksView(*this), m_actorView(*this), //m_worldParamatersView(*this),
	m_editRealityView(*this), m_editActorView(*this), m_editAreaView(*this), m_editFactionView(*this), m_editFactionsView(*this), 
	m_editStockPileView(*this), m_editDramaView(*this), m_minimumTimePerFrame(200), 
	m_minimumTimePerStep((int)(1000.f / (float)Config::stepsPerSecond)), m_draw(*this),
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
	}), m_editMode(false)
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
			gameView = &m_lastViewedSpotInArea.at(area.m_id);
		else
		{
			auto pair = m_lastViewedSpotInArea.try_emplace(area.m_id, &area.getMiddleAtGroundLevel(), displayData::defaultScale);
			assert(pair.second);
			gameView = &pair.first->second;
		}
	}
	m_area = &area;
	m_scale = gameView->scale;
	centerView(*gameView->center);
}
void Window::centerView(const Block& block)
{
	m_z = block.m_z;
	sf::Vector2f globalPosition(block.m_x * m_scale, block.m_y * m_scale);
	sf::Vector2i pixelPosition = m_window.mapCoordsToPixel(globalPosition);
	m_view.setCenter(pixelPosition.x, pixelPosition.y);
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
			uint32_t scrollSteps = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) ? 6 : 1;
			if(m_area)
				m_blockUnderCursor = &getBlockUnderCursor();
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
						case sf::Keyboard::PageUp:
							if(m_gameOverlay.isVisible() && (m_z + 1) < m_area->m_sizeZ)
								m_z += 1;
							break;
						case sf::Keyboard::PageDown:
							if(m_gameOverlay.isVisible() && m_z > 0)
								m_z -= 1;
							break;
						case sf::Keyboard::Up:
							if(m_gameOverlay.isVisible() && m_view.getCenter().y > (m_view.getSize().y / 2) - gameMarginSize)
								m_view.move(0.f, scrollSteps * -20.f);
							break;
						case sf::Keyboard::Down:
							if(m_gameOverlay.isVisible() && m_view.getCenter().y < m_area->m_sizeY * m_scale - (m_view.getSize().y / 2) + gameMarginSize)
								m_view.move(0.f, scrollSteps * 20.f);
							break;
						case sf::Keyboard::Left:
							if(m_gameOverlay.isVisible() && m_view.getCenter().x > (m_view.getSize().x / 2) - gameMarginSize)
								m_view.move(scrollSteps * -20.f, 0.f);
							break;
						case sf::Keyboard::Right:
							if(m_gameOverlay.isVisible() && m_view.getCenter().x < m_area->m_sizeX * m_scale - (m_view.getSize().x / 2) + gameMarginSize)
								m_view.move(scrollSteps * 20.f, 0.f);
							break;
						case sf::Keyboard::Delete:
							{
								m_scale = std::max(1u, (int)m_scale - scrollSteps);
								Block& center = *m_blockUnderCursor;
								m_view.move(-1.f *center.m_x * scrollSteps, -1.f * center.m_y * scrollSteps);
							}
							break;
						case sf::Keyboard::Insert:
							{
								m_scale += 1 * scrollSteps;
								Block& center = *m_blockUnderCursor;
								m_view.move(center.m_x * scrollSteps, center.m_y * scrollSteps);
							}
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
								else if(m_gameOverlay.m_itemBeingInstalled)
									m_gameOverlay.m_itemBeingInstalled = nullptr;
								else if(m_gameOverlay.m_itemBeingMoved)
									m_gameOverlay.m_itemBeingMoved = nullptr;
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
								showActorDetail(**m_selectedActors.begin());
							break;
						case sf::Keyboard::Num6:
							if(m_area && m_selectedActors.size() == 1)
								showObjectivePriority(**m_selectedActors.begin());
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
							m_firstCornerOfSelection = &getBlockAtPosition({event.mouseButton.x, event.mouseButton.y});
							m_positionWhereMouseDragBegan = {event.mouseButton.x, event.mouseButton.y};
							if(!sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
								deselectAll();
						}
					}
					break;
				}
				case sf::Event::MouseButtonReleased:
				{
					if(m_area)
					{
						Block& block = *m_blockUnderCursor;
						if(event.mouseButton.button == displayData::selectMouseButton)
						{
							
							Cuboid blocks;
							// Find the selected area.
							if(m_firstCornerOfSelection)
								blocks.setFrom(*m_firstCornerOfSelection, block);
							else
								blocks.setFrom(block);
							m_firstCornerOfSelection = nullptr;
							bool selectActors = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt);
							bool selectItems = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt);
							bool selectBlocks = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt);
							bool selectPlants = false;
							if(selectActors)
								for(Block& block : blocks)
									m_selectedActors.insert(block.m_hasActors.getAll().begin(), block.m_hasActors.getAll().end());
							else if(selectItems)
								for(Block& block : blocks)
									m_selectedItems.insert(block.m_hasItems.getAll().begin(), block.m_hasItems.getAll().end());
							else if(selectBlocks)
							{
								for(Block& block : blocks)
									m_selectedBlocks.insert(&block);
							}
							else
							{
								// No modifier key to choose what type to select, check for everything.
								std::unordered_set<Actor*> foundActors;
								std::unordered_set<Item*> foundItems;
								std::unordered_set<Plant*> foundPlants;
								// Gather all candidates, then cull based on choosen category. actors > items > plants > blocks
								for(Block& block : blocks)
								{
									if(!block.m_hasActors.empty())
									{
										m_selectedActors.insert(block.m_hasActors.getAll().begin(), block.m_hasActors.getAll().end());
										selectActors = true;
									}
									if(!block.m_hasItems.empty())
									{
										m_selectedItems.insert(block.m_hasItems.getAll().begin(), block.m_hasItems.getAll().end());
										selectItems = selectActors ? false : true;
									}
									if(block.m_hasPlant.exists())
									{
										m_selectedPlants.insert(&block.m_hasPlant.get());
										selectPlants = selectItems ? false : (selectActors ? false : true);
									}
								}
								if(selectActors)
								{
									m_selectedItems.clear();
									m_selectedPlants.clear();
									m_selectedBlocks.clear();
								}
								else if(selectItems)
								{
									m_selectedActors.clear();
									m_selectedPlants.clear();
									m_selectedBlocks.clear();
								}
								else if(selectPlants)
								{
									m_selectedActors.clear();
									m_selectedItems.clear();
									m_selectedBlocks.clear();
								}
								else
								{
									m_selectedActors.clear();
									m_selectedItems.clear();
									m_selectedPlants.clear();
									for(Block& block : blocks)
										m_selectedBlocks.insert(&block);
								}
							}
						}
						else if(event.mouseButton.button == displayData::actionMouseButton)
						{
							// Display context menu.
							//TODO: Left click and drag shortcut for select and open context.
							if(
									m_gameOverlay.m_itemBeingInstalled &&
									block.m_hasShapes.canEnterEverWithFacing(*m_gameOverlay.m_itemBeingInstalled, m_gameOverlay.m_facing)
							)
								m_gameOverlay.assignLocationToInstallItem(block);
							else if(m_gameOverlay.m_itemBeingMoved)
							{
								if(getSelectedActors().empty())
									m_gameOverlay.m_itemBeingMoved = nullptr;
								else if(block.m_hasShapes.canEnterEverWithFacing(*m_gameOverlay.m_itemBeingMoved, m_gameOverlay.m_facing))
									m_gameOverlay.assignLocationToMoveItemTo(block);
							}
							else
								m_gameOverlay.drawContextMenu(block);
						}
						else
							m_gameOverlay.closeContextMenu();
					}
					break;
				}
				default:
					if(m_area)
					{
						const Block& block = *m_blockUnderCursor;
						m_gameOverlay.m_coordinateUI->setText(
							std::to_string(block.m_x) + "," +
							std::to_string(block.m_y) + "," +
							std::to_string(block.m_z)
						);
					}
					break;
			}
		}
		// Clear sprites generated for previous frame.
		sprites::flush();
		// Begin draw.
		m_window.clear();
		// Draw main m_view.
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
			m_window.setView(m_view);
			m_draw.view();
			// What is this? Something for TGUI?
			m_window.setView(m_window.getDefaultView());
		}
		// Draw UI.
		m_gui.draw();
		// Finalize draw.
		m_window.display();
		// Frame rate limit.
		std::chrono::milliseconds delta = msSinceEpoch() - start;
		std::chrono::milliseconds minimum = m_speed < 200 ? m_minimumTimePerFrame : m_minimumTimePerFrame * 3;
		if(minimum > delta)
			std::this_thread::sleep_for(minimum - delta);
	}
}
void Window::threadTask(std::function<void()> task)
{
	m_lockInput = true;
	sf::Cursor cursor;
	if (cursor.loadFromSystem(sf::Cursor::Wait))
		m_window.setMouseCursor(cursor);
	std::thread t([this, task]{ 
		task();
		m_lockInput = false;
		sf::Cursor cursor;
		if (cursor.loadFromSystem(sf::Cursor::Arrow))
			m_window.setMouseCursor(cursor);
	});
	t.join();
}
void Window::save()
{
	assert(m_simulation);
	std::function<void()> task = [this]{
		m_simulation->save();
		const Json povData = povToJson();
		std::ofstream file(m_simulation->getPath()/"pov.json" );
		file << povData;
	};
	threadTask(task);
}
void Window::load(std::filesystem::path path)
{
	std::function<void()> task = [this, path]{
		deselectAll();
		m_simulation = std::make_unique<Simulation>(path);
		std::ifstream af(m_simulation->getPath()/"pov.json");
		Json povData = Json::parse(af);
		povFromJson(povData);
		showGame();
	};
	threadTask(task);
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
void Window::setSpeed(uint16_t speed)
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
		m_faction = &m_simulation->m_hasFactions.byName(data["faction"].get<std::wstring>());
	m_area = &m_simulation->m_hasAreas->getById(data["area"].get<AreaId>());
	m_scale = data["scale"].get<uint32_t>();
	m_z = data["z"].get<uint32_t>();
	uint32_t x = data["x"].get<uint32_t>();
	uint32_t y = data["y"].get<uint32_t>();
	m_view.setCenter(x, y);
}
void Window::deselectAll()
{
	m_selectedBlocks.clear();
	m_selectedItems.clear();
	m_selectedActors.clear();
	m_selectedPlants.clear();
}
void Window::selectBlock(Block& block)
{
	//TODO: additive select.
	deselectAll();
	m_selectedBlocks.insert(&block);
}
void Window::selectItem(Item& item)
{
	deselectAll();
	m_selectedItems.insert(&item);
}
void Window::selectPlant(Plant& plant)
{
	deselectAll();
	m_selectedPlants.insert(&plant);
}
void Window::selectActor(Actor& actor)
{
	deselectAll();
	m_selectedActors.insert(&actor);
}
Block& Window::getBlockUnderCursor()
{
	sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
	return getBlockAtPosition(pixelPos);
}
Block& Window::getBlockAtPosition(sf::Vector2i pixelPos)
{
	sf::Vector2f worldPos = m_window.mapPixelToCoords(pixelPos);
	uint32_t x = std::max(0.f, worldPos.x + m_view.getCenter().x - m_view.getSize().x / 2);
	uint32_t y = std::max(0.f, worldPos.y + m_view.getCenter().y - m_view.getSize().y / 2);
	x = std::min(m_area->m_sizeX - 1, x / m_scale);
	y = std::min(m_area->m_sizeY - 1, y / m_scale);
	return m_area->getBlock(x, y, m_z);
}
[[nodiscard]] Json Window::povToJson() const
{
	Json data{
		{"area", m_area},
		{"scale", m_scale},
		{"z", m_z},
		{"x", m_view.getCenter().x},
		{"y", m_view.getCenter().y}
	};
	if(m_faction)
		data["faction"] = m_faction;
	return data;
}
// static method
std::chrono::milliseconds Window::msSinceEpoch()
{
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}
std::wstring Window::displayNameForItem(const Item& item)
{
	std::wstring output;
	if(!item.m_name.empty())
	{
		output.append(item.m_name);
		output.append(L" a ");
	}
	output.append(util::stringToWideString(item.m_materialType.name));
		output.append(L" ");
	output.append(util::stringToWideString(item.m_itemType.name));
	return output;
}
std::wstring Window::displayNameForCraftJob(CraftJob& craftJob)
{
	std::wstring output;
	output.append(util::stringToWideString(craftJob.craftJobType.name));
	if(craftJob.materialType)
	{
		output.append(L" ");
		output.append(util::stringToWideString(craftJob.materialType->name));
	}
	return output;
}
std::string Window::facingToString(Facing facing)
{
	if(facing == 0)
		return "up";
	if(facing == 1)
		return "right";
	if(facing == 2)
		return "down";
	assert(facing == 3);
	return "left";
}
