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
constexpr sf::Mouse::Button selectMouseButton = sf::Mouse::Button::Left;
constexpr sf::Mouse::Button actionMouseButton = sf::Mouse::Button::Right;
Window::Window() : m_window(sf::VideoMode::getDesktopMode(), "Goblin Pit", sf::Style::Fullscreen), m_gui(m_window), m_view(m_window.getDefaultView()), 
	m_mainMenuView(*this), m_loadView(*this), m_gameOverlay(*this), m_objectivePriorityView(*this), 
	m_productionView(*this), m_uniformView(*this), m_stocksView(*this), m_actorView(*this), //m_worldParamatersView(*this),
	m_editRealityView(*this), m_editActorView(*this), m_editAreaView(*this), m_editFactionView(*this), m_editFactionsView(*this), 
	m_editStockPileView(*this), m_editDramaView(*this), m_area(nullptr), m_scale(32), m_z(0), m_speed(1), m_faction(nullptr),
	m_minimumTimePerFrame(200), m_minimumTimePerStep((int)(1000.f / (float)Config::stepsPerSecond)), m_draw(*this),
	m_simulationThread([&](){
		while(true)
		{
			std::chrono::milliseconds start;
			if(m_speed)
				start = msSinceEpoch();
			if(m_simulation && !m_paused)
				m_simulation->doStep();
			if(m_speed)
			{
				std::chrono::milliseconds delta = msSinceEpoch() - start;
				std::chrono::milliseconds adjustedMinimum = std::chrono::milliseconds(m_minimumTimePerStep / std::chrono::milliseconds(m_speed));
				if(delta < adjustedMinimum)
					std::this_thread::sleep_for(adjustedMinimum - delta);
			}
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
	pixelPosition.x -= m_view.getCenter().x - m_view.getSize().x / 2;
	pixelPosition.y -= m_view.getCenter().y - m_view.getSize().y / 2;
	if(pixelPosition.x < 50 && m_view.getCenter().x > (m_view.getSize().x / 2) - 10)
		m_view.move(-20.f, 0.f);
	else if (pixelPosition.x > m_view.getSize().x - 50 && m_view.getCenter().x < m_area->m_sizeX * m_scale - (m_view.getSize().x / 2) + 10)
		m_view.move(20.f, 0.f);
	else if(pixelPosition.y < 50 && m_view.getCenter().y > (m_view.getSize().y / 2) - 10)
		m_view.move(0.f, -20.f);
	else if (pixelPosition.y > m_view.getSize().y - 50 && m_view.getCenter().y < m_area->m_sizeY * m_scale - (m_view.getSize().y / 2) + 10)
		m_view.move(0.f, 20.f);
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
								Block& center = getBlockUnderCursor();
								m_view.move(-1.f *center.m_x * scrollSteps, -1.f * center.m_y * scrollSteps);
							}
							break;
						case sf::Keyboard::Insert:
							{
								m_scale += 1 * scrollSteps;
								Block& center = getBlockUnderCursor();
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
								setSpeed(2);
							break;
						case sf::Keyboard::F3:
							if(m_area)
								setSpeed(4);
							break;
						case sf::Keyboard::F4:
							if(m_area)
								setSpeed(8);
							break;
						case sf::Keyboard::F5:
							if(m_area)
								setSpeed(16);
							break;
						case sf::Keyboard::F6:
							if(m_area)
								setSpeed(32);
							break;
						case sf::Keyboard::F7:
							if(m_area)
								setSpeed(64);
							break;
						case sf::Keyboard::F8:
							if(m_area)
								setSpeed(128);
							break;
						case sf::Keyboard::F9:
							if(m_area)
								setSpeed(0);
							break;
						default:
							break;
					}
					break;
				case sf::Event::MouseButtonPressed:
				{
					if(m_area)
					{
						if(event.mouseButton.button == selectMouseButton)
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
						Block& block = getBlockUnderCursor();
						if(event.mouseButton.button == selectMouseButton)
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
						else if(event.mouseButton.button == actionMouseButton)
						{
							// Display context menu.
							//TODO: Left click and drag shortcut for select and open context.
							if(m_gameOverlay.m_itemBeingInstalled)
								m_gameOverlay.installItem(block);
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
						const Block& block = getBlockUnderCursor();
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
			m_window.setView(m_view);
			drawView();
			// What is this? Something for TGUI?
			m_window.setView(m_window.getDefaultView());
		}
		// Draw UI.
		m_gui.draw();
		// Finalize draw.
		m_window.display();
		// Frame rate limit.
		std::chrono::milliseconds delta = msSinceEpoch() - start;
		std::chrono::duration ms = m_speed ? m_minimumTimePerFrame : m_minimumTimePerFrame * 2;
		if(ms > delta)
			std::this_thread::sleep_for(ms - delta);
	}
}
void Window::drawView()
{
	m_gameOverlay.drawTime();
	//m_gameOverlay.drawWeather();
	// Aquire Area read mutex.
	std::lock_guard lock(m_simulation->m_uiReadMutex);
	// Render block floors, collect actors into single and multi tile groups.
	std::unordered_set<const Block*> singleTileActorBlocks;
	std::unordered_set<const Actor*> multiTileActors;
	for(Block& block : m_area->getZLevel(m_z))
	{
		m_draw.blockFloor(block);
		for(const Actor* actor : block.m_hasActors.getAllConst())
		{
			if(actor->m_shape->isMultiTile)
				multiTileActors.insert(actor);
			else
				singleTileActorBlocks.insert(&block);
		}
	}
	// Render block wall corners.
	for(Block& block : m_area->getZLevel(m_z))
		m_draw.blockWallCorners(block);
	// Render block walls.
	for(Block& block : m_area->getZLevel(m_z))
		m_draw.blockWalls(block);
	// Render block features and fluids.
	for(Block& block : m_area->getZLevel(m_z))
		m_draw.blockFeaturesAndFluids(block);
	// Render block plants.
	for(Block& block : m_area->getZLevel(m_z))
		m_draw.nonGroundCoverPlant(block);
	// Render items.
	for(Block& block : m_area->getZLevel(m_z))
		m_draw.item(block);
	// Render block wall tops.
	for(Block& block : m_area->getZLevel(m_z))
		m_draw.blockWallTops(block);
	// Render Actors.
	// Do multi tile actors first.
	//TODO: what if multitile actors overlap?
	for(const Actor* actor : multiTileActors)
		m_draw.multiTileActor(*actor);
	// Do single tile actors.
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	for(const Block* block : singleTileActorBlocks)
	{
		assert(!block->m_hasActors.empty());
		// Multiple actors, cycle through them over time.
		// Use real time rather then m_step to continue cycling while paused.
		uint32_t count = block->m_hasActors.getAllConst().size();
		if(count == 1)
			m_draw.singleTileActor(*block->m_hasActors.getAllConst().front());
		else
		{
			uint8_t index = (seconds % count);
			//TODO: hide some actors from player?
			const std::vector<Actor*> actors = block->m_hasActors.getAllConst();
			m_draw.singleTileActor(*actors[index]);
		}
	}
	// Designated and project progress.
	if(m_faction)
		for(Block& block : m_area->getZLevel(m_z))
		{
			if(block.m_hasDesignations.containsFaction(*m_faction))
				m_draw.designated(block);
			m_draw.craftLocation(block);
			Percent projectProgress = block.m_hasProjects.getProjectPercentComplete(*m_faction);
			if(projectProgress)
				m_draw.progressBarOnBlock(block, projectProgress);
		}
	// Selected.
	if(!m_selectedBlocks.empty())
	{
		for(Block* block : m_selectedBlocks)
			if(block->m_z == m_z)
				m_draw.selected(*block);
	}
	else if(!m_selectedActors.empty())
	{
		for(Actor* actor: m_selectedActors)
			for(Block* block : actor->m_blocks)
				if(block->m_z == m_z)
					m_draw.selected(*block);
	}
	else if(!m_selectedItems.empty())
	{
		for(Item* item: m_selectedItems)
			for(Block* block : item->m_blocks)
				if(block->m_z == m_z)
					m_draw.selected(*block);
	}
	else if(!m_selectedPlants.empty())
		for(Plant* plant: m_selectedPlants)
			for(Block* block : plant->m_blocks)
				if(block->m_z == m_z)
					m_draw.selected(*block);
	// Selection Box.
	if(m_firstCornerOfSelection && sf::Mouse::isButtonPressed(selectMouseButton))
	{
		auto end = sf::Mouse::getPosition();
		auto start = m_positionWhereMouseDragBegan;
		uint32_t xSize = std::abs((int32_t)start.x - (int32_t)end.x);
		uint32_t ySize = std::abs((int32_t)start.y - (int32_t)end.y);
		int32_t left = std::min(start.x, end.x);
		int32_t top = std::min(start.y, end.y);
		sf::Vector2f worldPos = m_window.mapPixelToCoords({left, top});
		sf::RectangleShape square(sf::Vector2f(xSize, ySize));
		square.setFillColor(sf::Color::Transparent);
		square.setOutlineColor(displayData::selectColor);
		square.setOutlineThickness(3.f);
		square.setPosition(worldPos);
		m_window.draw(square);
	}
	// Install item.
	if(m_gameOverlay.m_itemBeingInstalled)
	{
		Block& hoverBlock = getBlockUnderCursor();
		auto blocks = m_gameOverlay.m_itemBeingInstalled->getBlocksWhichWouldBeOccupiedAtLocationAndFacing(hoverBlock, m_gameOverlay.m_facing);
		bool valid = hoverBlock.m_hasShapes.canEnterEverWithFacing(*m_gameOverlay.m_itemBeingInstalled, m_gameOverlay.m_facing);
		for(Block* block : blocks)
			if(!valid)
				m_draw.invalidOnBlock(*block);
			else
				m_draw.validOnBlock(*block);
	}
	// Area Border.
	sf::RectangleShape areaBorder(sf::Vector2f((m_scale * m_area->m_sizeX), (m_scale * m_area->m_sizeX) ));
	areaBorder.setOutlineColor(sf::Color::White);
	areaBorder.setFillColor(sf::Color::Transparent);
	areaBorder.setOutlineThickness(3.f);
	areaBorder.setPosition(sf::Vector2f(0,0));
	m_window.draw(areaBorder);
	// Update Info popup.
	//if(m_gameOverlay.infoPopupIsVisible())
		//m_gameOverlay.updateInfoPopup();
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
void Window::setSpeed(uint8_t speed)
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
	m_area = &m_simulation->getAreaById(data["area"].get<AreaId>());
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
	output.append(util::stringToWideString(craftJob.materialType->name));
	output.append(L" ");
	output.append(util::stringToWideString(craftJob.craftJobType.name));
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
