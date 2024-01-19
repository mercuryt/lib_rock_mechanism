#include "window.h"
#include "displayData.h"
#include <SFML/Window/Keyboard.hpp>
#include <TGUI/Widgets/FileDialog.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <ratio>
#include <mutex>
#include <unordered_set>
constexpr sf::Mouse::Button selectMouseButton = sf::Mouse::Button::Left;
constexpr sf::Mouse::Button actionMouseButton = sf::Mouse::Button::Right;
sf::Color selectColor = sf::Color::Yellow;
Window::Window() : m_window(sf::VideoMode::getDesktopMode(), "Goblin Pit", sf::Style::Fullscreen), m_gui(m_window), m_view(m_window.getDefaultView()), 
	m_mainMenuView(*this), m_loadView(*this), m_gameOverlay(*this), m_objectivePriorityView(*this), 
	m_productionView(*this), m_uniformView(*this), m_stocksView(*this), m_actorView(*this),
	m_simulation(nullptr), m_faction(nullptr),
	m_paused(true), m_minimumMillisecondsPerFrame(200), m_minimumMillisecondsPerStep(200),
	m_simulationThread([&](){
		std::chrono::milliseconds start = msSinceEpoch();
		if(m_simulation && !m_paused)
			m_simulation->doStep();
		std::chrono::milliseconds delta = msSinceEpoch() - start;
		if(delta < m_minimumMillisecondsPerStep)
			std::this_thread::sleep_for(m_minimumMillisecondsPerStep - delta);
	})
{
	m_font.loadFromFile("lib/arial.ttf");
	showMainMenu();
}
void Window::setArea(Area& area, Block& center, uint32_t scale)
{
	m_area = &area;
	m_scale = scale;
	centerView(center);
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
	m_loadView.hide();
	m_objectivePriorityView.hide();
	m_productionView.hide();
	m_uniformView.hide();
	m_worldParamatersView.hide();
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
						case sf::Keyboard::PageUp:
							if(m_gameOverlay.isVisible() && (m_z + 1) < m_area->m_sizeZ)
								++m_z;
							break;
						case sf::Keyboard::PageDown:
							if(m_gameOverlay.isVisible() && m_z > 0)
								--m_z;
							break;
						case sf::Keyboard::Up:
							if(m_gameOverlay.isVisible() && m_view.getCenter().y > (m_view.getSize().y / 2) - 10)
								m_view.move(0.f, scrollSteps * -20.f);
							break;
						case sf::Keyboard::Down:
							if(m_gameOverlay.isVisible() && m_view.getCenter().y < m_area->m_sizeY * m_scale - (m_view.getSize().y / 2) + 10)
								m_view.move(0.f, scrollSteps * 20.f);
							break;
						case sf::Keyboard::Left:
							if(m_gameOverlay.isVisible() && m_view.getCenter().x > (m_view.getSize().x / 2) - 10)
								m_view.move(scrollSteps * -20.f, 0.f);
							break;
						case sf::Keyboard::Right:
							if(m_gameOverlay.isVisible() && m_view.getCenter().x < m_area->m_sizeX * m_scale - (m_view.getSize().x / 2) + 10)
								m_view.move(scrollSteps * 20.f, 0.f);
							break;
						case sf::Keyboard::Period:
							if(m_gameOverlay.isVisible() && m_paused)
								m_simulation->doStep();
							break;
						case sf::Keyboard::Space:
							if(m_gameOverlay.isVisible())
								m_paused = !m_paused;
							break;
						case sf::Keyboard::Escape:
							if(m_gameOverlay.isVisible())
							{
								if(m_gameOverlay.menuIsVisible())
									m_gameOverlay.closeMenu();
								else if(m_gameOverlay.contextMenuIsVisible())
									m_gameOverlay.closeContextMenu();
								else if(m_gameOverlay.detailPanelIsVisible())
									m_gameOverlay.closeDetailPanel();
								else if(m_gameOverlay.m_itemBeingInstalled)
									m_gameOverlay.m_itemBeingInstalled = nullptr;
								else
									m_gameOverlay.drawMenu();
							}
							if(m_productionView.isVisible())
								if(m_productionView.createIsVisible())
									m_productionView.closeCreate();
								else
									showGame();
							else
								showGame();
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
						default:
							break;
					}
					break;
				case sf::Event::MouseButtonPressed:
				{
					if(m_area)
					{
						Block& block = getBlockUnderCursor();
						if(event.mouseButton.button == selectMouseButton)
						{
							m_firstCornerOfSelection = &block;
							m_positionWhereMouseDragBegan = sf::Mouse::getPosition();
							//TODO: Additive select.
							m_selectedBlocks.clear();
							m_selectedActors.clear();
							m_selectedItems.clear();
							m_selectedPlants.clear();
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
							bool selectActors = sf::Keyboard::isKeyPressed(sf::Keyboard::LShift);
							bool selectItems = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && !sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt);
							bool selectPlants = sf::Keyboard::isKeyPressed(sf::Keyboard::LControl) && sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt);
							bool selectBlocks = sf::Keyboard::isKeyPressed(sf::Keyboard::LAlt);
							if(selectActors)
								for(Block& block : blocks)
									m_selectedActors.insert(block.m_hasActors.getAll().begin(), block.m_hasActors.getAll().end());
							else if(selectItems)
								for(Block& block : blocks)
									m_selectedItems.insert(block.m_hasItems.getAll().begin(), block.m_hasItems.getAll().end());
							else if(selectPlants)
								for(Block& block : blocks)
									m_selectedPlants.insert(&block.m_hasPlant.get());
							else if(selectBlocks)
								m_selectedBlocks = blocks;
							else
							{
								// No modifier key to choose what type to select, check for everything.
								std::unordered_set<Actor*> foundActors;
								std::unordered_set<Item*> foundItems;
								std::unordered_set<Plant*> foundPlants;
								for(Block& block : blocks)
								{
									m_selectedActors.insert(block.m_hasActors.getAll().begin(), block.m_hasActors.getAll().end());
									m_selectedItems.insert(block.m_hasItems.getAll().begin(), block.m_hasItems.getAll().end());
									m_selectedPlants.insert(&block.m_hasPlant.get());
								}
								if(!foundActors.empty())
									m_selectedActors = foundActors;
								else if(!foundItems.empty())
									m_selectedItems = foundItems;
								else if(!foundPlants.empty())
									m_selectedPlants = foundPlants;
								else
									m_selectedBlocks = blocks;
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
					}
					break;
				}
				default:
					break;
			}
		}
		// Begin draw.
		m_window.clear();
		// Draw main m_view.
		if(m_simulation && m_area)
		{
			m_window.setView(m_view);
			drawView();
			// What is this?
			m_window.setView(m_window.getDefaultView());
		}
		// Draw UI.
		m_gui.draw();
		// Finalize draw.
		m_window.display();
		// Frame rate limit.
		std::chrono::milliseconds delta = msSinceEpoch() - start;
		if(m_minimumMillisecondsPerFrame > delta)
			std::this_thread::sleep_for(m_minimumMillisecondsPerFrame - delta);
	}
}
void Window::drawView()
{
	// Aquire Area read mutex.
	std::lock_guard lock(m_simulation->m_uiReadMutex);
	// Render blocks, collect actors into single and multi tile groups.
	std::unordered_set<const Block*> singleTileActorBlocks;
	std::unordered_set<const Actor*> multiTileActors;
	for(Block& block : m_area->getZLevel(m_z))
	{
		drawBlock(block);
		for(const Actor* actor : block.m_hasActors.getAllConst())
		{
			if(actor->m_shape->isMultiTile)
				multiTileActors.insert(actor);
			else
				singleTileActorBlocks.insert(&block);
		}
	}
	// Render Actors.
	// Do multi tile actors first.
	//TODO: what if multitile actors overlap?
	for(const Actor* actor : multiTileActors)
		drawActor(*actor);
	// Do single tile actors.
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
	for(const Block* block : singleTileActorBlocks)
	{
		assert(!block->m_hasActors.empty());
		// Multiple actors, cycle through them over time.
		// Use real time rather then m_step to continue cycling while paused.
		uint32_t count = block->m_hasActors.getAllConst().size();
		uint8_t modulo = seconds % count;
		//TODO: hide some actors from player?
		const std::vector<Actor*> actors = block->m_hasActors.getAllConst();
		drawActor(*actors[modulo]);
	}
	// Selection Box.
	if(m_firstCornerOfSelection && sf::Mouse::isButtonPressed(selectMouseButton))
	{
		auto end = sf::Mouse::getPosition();
		auto start = m_positionWhereMouseDragBegan;
		uint32_t xSize = std::abs((int32_t)start.x - (int32_t)end.x);
		uint32_t ySize = std::abs((int32_t)start.y - (int32_t)end.y);
		uint32_t left = std::min(start.x, end.x);
		uint32_t top = std::min(start.y, end.y);
		sf::RectangleShape square(sf::Vector2f(xSize, ySize));
		square.setFillColor(sf::Color::Transparent);
		square.setOutlineColor(selectColor);
		square.setOutlineThickness(3.f);
		square.setPosition(left, top);
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
				drawInvalidOnBlock(*block);
			else
				drawValidOnBlock(*block);
	}
}
void Window::drawBlock(const Block& block)
{
	if(block.isSolid())
	{
		for(Block* adjacent : block.m_adjacentsVector)
		{
			if(adjacent->m_visible)
			{
				drawColorOnBlock(block, displayData::materialColors.at(&block.getSolidMaterial()));
				sf::RectangleShape square(sf::Vector2f(m_scale, m_scale));
				square.setFillColor(sf::Color::White);
				square.setPosition(static_cast<float>(block.m_x * m_scale), static_cast<float>(block.m_y * m_scale));
				m_window.draw(square);
				break;
			}
		}
	}
	else
	{
		// If the last bit of the block seed address is set.
		/*
		if(block.m_seed & (1<<1))
		{
			if(block.m_seed & (1<<1))
				text.setString('.');
			else
				text.setString(':');
		}
		else
		{
			if(block.m_seed & (1<<1))
				text.setString(',');
			else
				text.setString(';');
		}
		*/
		// Block Features
		//TODO: Draw order.
		for(const BlockFeature& blockFeature : block.m_hasBlockFeatures.get())
		{
			auto string = displayData::blockFeatureSymbols.at(blockFeature.blockFeatureType);
			sf::Color color = displayData::materialColors.at(blockFeature.materialType);
			drawStringOnBlock(block, string, color);
		}
		// Fluids
		if(block.m_totalFluidVolume)
		{
			const FluidType& fluidType = block.getFluidTypeWithMostVolume();
			Volume volume = block.m_fluids.at(&fluidType).first;
			sf::Color color = displayData::fluidColors.at(&fluidType);
			drawStringOnBlock(block, std::to_wstring(volume), color);
		}
	}
	if(m_selectedBlocks.contains(block))
		drawOutlineOnBlock(block, selectColor);
}
void Window::drawValidOnBlock(const Block& block)
{
	drawColorOnBlock(block, {0, 255, 0, 122});
}
void Window::drawInvalidOnBlock(const Block& block)
{
	drawColorOnBlock(block, {255, 0, 0, 122});
}
void Window::drawColorOnBlock(const Block& block, sf::Color color)
{
	sf::RectangleShape square(sf::Vector2f(m_scale, m_scale));
	square.setFillColor(color);
	square.setPosition(static_cast<float>(block.m_x * m_scale), static_cast<float>(block.m_y * m_scale));
	m_window.draw(square);
}
void Window::drawOutlineOnBlock(const Block& block, sf::Color color, float thickness)
{
	sf::RectangleShape square(sf::Vector2f(m_scale, m_scale));
	square.setFillColor(sf::Color::Transparent);
	square.setOutlineColor(color);
	square.setOutlineThickness(thickness);
	square.setPosition(static_cast<float>(block.m_x * m_scale), static_cast<float>(block.m_y * m_scale));
	m_window.draw(square);
}
void Window::drawStringOnBlock(const Block& block, std::wstring string, sf::Color color)
{

	sf::Text text;
	text.setFont(m_font);
	text.setFillColor(color);
	text.setCharacterSize(10);
	text.setString(string);
	text.setPosition(static_cast<float>(block.m_x * m_scale), static_cast<float>(block.m_y * m_scale));
	//text.setScale(static_cast<float>(m_scale), static_cast<float>(m_scale));
	m_window.draw(text);
}
void Window::drawActor(const Actor& actor)
{
	auto str = displayData::actorSymbols.at(&actor.m_species);
	// TODO: Toggle team colors, job colors, weapon border.
	sf::Color color = displayData::actorColors.at(&actor.m_species);
	drawStringOnBlock(*actor.m_location, str, color);
	if(m_selectedActors.contains(&const_cast<Actor&>(actor)))
		for(Block* occupied : actor.m_blocks)
			drawOutlineOnBlock(*occupied, selectColor);
}
void Window::drawItem(const Item& item)
{
	auto str = displayData::itemSymbols.at(&item.m_itemType);
	sf::Color color = displayData::materialColors.at(&item.m_materialType);
	drawStringOnBlock(*item.m_location, str, color);
	if(m_selectedItems.contains(&const_cast<Item&>(item)))
		for(Block* occupied : item.m_blocks)
			drawOutlineOnBlock(*occupied, selectColor);
}
void Window::drawPlant(const Plant& plant)
{
	auto str = displayData::plantSymbols.at(&plant.m_plantSpecies);
	drawStringOnBlock(*plant.m_location, str, sf::Color::Green);
	if(m_selectedPlants.contains(&const_cast<Plant&>(plant)))
		for(Block* occupied : plant.m_blocks)
			drawOutlineOnBlock(*occupied, selectColor);
}
void Window::save()
{
	assert(m_simulation);
	m_simulation->save(m_path);
	const Json povData = povToJson();
	std::ofstream file(m_path/"pov.json");
	file << povData;
}
void Window::load(std::filesystem::path path)
{
	m_path = path;
	m_simulation = std::make_unique<Simulation>(m_path);
	std::ifstream af(m_path/"pov.json");
	Json povData = Json::parse(af);
	povFromJson(povData);
	showGame();
}
void Window::povFromJson(const Json& data)
{
	assert(m_simulation);
	m_area = &m_simulation->getAreaById(data["area"].get<AreaId>());
	m_scale = data["scale"].get<uint32_t>();
	m_z = data["m_z"].get<uint32_t>();
	if(data.contains("selectedBlock"))
	{
		uint32_t x = data["x"].get<uint32_t>();
		uint32_t y = data["y"].get<uint32_t>();
		m_view.setCenter(x, y);
	}
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
	m_selectedBlocks.setFrom(block);
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
	sf::Vector2f worldPos = m_window.mapPixelToCoords(pixelPos);
	uint32_t x = worldPos.x + m_view.getCenter().x - m_view.getSize().x / 2;
	uint32_t y = worldPos.y + m_view.getCenter().y - m_view.getSize().y / 2;
	x = std::min(m_area->m_sizeX - 1, x / m_scale);
	y = std::min(m_area->m_sizeY - 1, y / m_scale);
	return m_area->getBlock(x, y, m_z);
}
[[nodiscard]] Json Window::povToJson() const
{
	Json data{
		{"area", m_area},
		{"scale", m_scale},
		{"z", m_z}
	};
	data["x"] = m_view.getCenter().x;
	data["y"] = m_view.getCenter().y;
	return data;
}
// static method
std::chrono::milliseconds Window::msSinceEpoch()
{
	auto duration = std::chrono::system_clock::now().time_since_epoch();
	return std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}
std::wstring Window::displayNameForItem(Item& item)
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
