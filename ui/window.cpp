#include "window.h"
#include <TGUI/Widgets/FileDialog.hpp>
#include <chrono>
#include <filesystem>
#include <memory>
#include <ratio>
#include <shared_mutex>
Window::Window() : m_window(sf::VideoMode(600, 600), "Goblin Pit", sf::Style::Fullscreen), m_view(m_window.getDefaultView()), m_gui(m_window), 
	m_mainMenuGui(tgui::Group::create()), m_loadGui(tgui::Group::create()), m_gameGui(tgui::Group::create()), m_objectivePriorityGui(tgui::Group::create()), m_detailPanel(*this, m_gameGui),
	m_simulation(nullptr), m_selectedBlock(nullptr), m_selectedItem(nullptr), m_selectedPlant(nullptr), m_selectedActor(nullptr), 
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
	// Create load screen.
	m_gui.add(m_loadGui);
	auto fileDialog = tgui::FileDialog::create("open file", "open");
	fileDialog->onFileSelect([this](const std::filesystem::path path){ load(path); });
	fileDialog->setPath("save");
	m_loadGui->add(fileDialog);
	auto cancelButton = tgui::Button::create("cancel");
	cancelButton->onPress(&Window::showMainMenu);
	m_loadGui->add(cancelButton);
	// Create game view.
	m_gui.add(m_gameGui);
	// Create objective priority view.
	m_gui.add(m_objectivePriorityGui);
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
	sf::Vector2f globalPosition(m_selectedBlock->m_x * m_scale, m_selectedBlock->m_y * m_scale);
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
						case sf::Keyboard::Escape:
							m_selectedBlock = nullptr;
							break;
						case sf::Keyboard::PageUp:
							if((m_z + 1) < m_area->m_sizeZ)
							{
								++m_z;
								if(m_selectedBlock)
									m_selectedBlock = m_selectedBlock->m_adjacents[5];
							}
							break;
						case sf::Keyboard::PageDown:
							if(m_z > 0)
							{
								--m_z;
								if(m_selectedBlock)
									m_selectedBlock = m_selectedBlock->m_adjacents[0];
							}
							break;
						case sf::Keyboard::Up:
							if(m_selectedBlock && m_selectedBlock->m_adjacents[3])
								m_selectedBlock = m_selectedBlock->m_adjacents[3];
							else if(m_view.getCenter().y > (m_view.getSize().y / 2) - 10)
								m_view.move(0.f, scrollSteps * -20.f);
							break;
						case sf::Keyboard::Down:
							if(m_selectedBlock && m_selectedBlock->m_adjacents[1])
								m_selectedBlock = m_selectedBlock->m_adjacents[1];
							else if(m_view.getCenter().y < m_area->m_sizeY * m_scale - (m_view.getSize().y / 2) + 10)
								m_view.move(0.f, scrollSteps * 20.f);
							break;
						case sf::Keyboard::Left:
							if(m_selectedBlock && m_selectedBlock->m_adjacents[2])
								m_selectedBlock = m_selectedBlock->m_adjacents[2];
							else if(m_view.getCenter().x > (m_view.getSize().x / 2) - 10)
								m_view.move(scrollSteps * -20.f, 0.f);
							break;
						case sf::Keyboard::Right:
							if(m_selectedBlock && m_selectedBlock->m_adjacents[4])
								m_selectedBlock = m_selectedBlock->m_adjacents[4];
							else if(m_view.getCenter().x < m_area->m_sizeX * m_scale - (m_view.getSize().x / 2) + 10)
								m_view.move(scrollSteps * 20.f, 0.f);
							break;
						case sf::Keyboard::Period:
							if(m_paused)
								m_simulation->doStep();
							break;
						case sf::Keyboard::Space:
							m_paused = !m_paused;
							break;
						default:
							assert(false);
					}
					break;
				case sf::Event::MouseButtonPressed:
				{
					sf::Vector2i pixelPos = sf::Mouse::getPosition(m_window);
					sf::Vector2f worldPos = m_window.mapPixelToCoords(pixelPos);
					uint32_t x = worldPos.x + m_view.getCenter().x - m_view.getSize().x / 2;
					uint32_t y = worldPos.y + m_view.getCenter().y - m_view.getSize().y / 2;
					x = std::min(m_area->m_sizeX - 1, x / m_scale);
					y = std::min(m_area->m_sizeY - 1, y / m_scale);
					m_selectedBlock = &m_area->getBlock(x, y, m_z);
					break;
				}
				default:
					assert(false);
			}
		}
		// Begin draw.
		m_window.clear();
		// Draw main m_view.
		m_window.setView(m_view);
		drawView();
		// Draw UI.
		m_window.setView(m_window.getDefaultView());
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
	std::shared_lock lock(m_simulation->m_uiReadMutex);
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
}
void Window::drawBlock(const Block& block)
{
	if(block.isSolid())
	{
		for(Block* adjacent : block.m_adjacentsVector)
		{
			if(adjacent->m_visible)
			{
				sf::RectangleShape square(sf::Vector2f(m_scale, m_scale));
				square.setFillColor(sf::Color::White);
				square.setPosition(static_cast<float>(block.m_x * m_scale), static_cast<float>(block.m_y * m_scale));
				m_window.draw(square);
				break;
			}
		}
		// Solid but not visible.
	}
	else
	{
		sf::Text text;
		text.setFont(m_font);
		text.setFillColor(sf::Color::White);
		text.setCharacterSize(10);
		// Block Features
		if(!block.m_hasBlockFeatures.empty())
		{
			if(block.m_hasBlockFeatures.contains(BlockFeatureType::stairs))
				text.setString(L"𝄛"); //Ξ ≣ ☰ ㄣ ䷀ 𐂪  
			else if(block.m_hasBlockFeatures.contains(BlockFeatureType::ramp))
				text.setString(L"𝀄");
			else if(block.m_hasBlockFeatures.contains(BlockFeatureType::floorGrate))
				text.setString(L"#");
			else if(block.m_hasBlockFeatures.contains(BlockFeatureType::door))
				text.setString(L"┼");
		}
		else if(block.m_adjacents[0] != nullptr && block.m_adjacents[0]->isSolid())
		{
			// If the last bit of the block seed address is set.
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

		}
		if(!text.getString().isEmpty())
			text.setPosition(static_cast<float>(block.m_seed % m_scale + (block.m_x * m_scale)), static_cast<float>(block.m_y * m_scale));
		text.setScale(static_cast<float>(m_scale), static_cast<float>(m_scale));
		m_window.draw(text);
	}
	if(m_selectedBlock == &block)
	{
		sf::RectangleShape square(sf::Vector2f(m_scale, m_scale));
		square.setFillColor(sf::Color::Transparent);
		square.setOutlineColor(sf::Color::Yellow);
		square.setOutlineThickness(3.f);
		square.setPosition(static_cast<float>(block.m_x * m_scale), static_cast<float>(block.m_y * m_scale));
		m_window.draw(square);
	}
}
void Window::drawActor(const Actor& actor)
{
	sf::Text text;
	text.setFont(m_font);
	text.setFillColor(sf::Color::Green);
	text.setCharacterSize(10);
	text.setString(L"😁");
	text.setPosition(static_cast<float>(actor.m_location->m_x * m_scale), static_cast<float>(actor.m_location->m_y * m_scale));
	//text.setScale(static_cast<float>(m_scale), static_cast<float>(m_scale));
	m_window.draw(text);
}
void Window::save(std::filesystem::path path)
{
	assert(m_simulation);
	m_simulation->save(path);
	const Json povData = povToJson();
	std::ofstream file(path/"pov.json");
	file << povData;
}
void Window::load(std::filesystem::path path)
{
	m_simulation = std::make_unique<Simulation>(path);
	std::ifstream af(path/"pov.json");
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
		uint32_t x = data["blockX"].get<uint32_t>();
		uint32_t y = data["blockY"].get<uint32_t>();
		m_selectedBlock = &m_area->getBlock(x, y, m_z);
	}
}
void Window::selectBlock(Block& block)
{
	if(m_selectedBlock == &block || m_detailPanel.isVisible())
		m_detailPanel.display(block);
	m_selectedBlock = &block;
	m_selectedItem = nullptr;
	m_selectedPlant = nullptr;
	m_selectedActor = nullptr;
}
void Window::selectItem(Item& item)
{
	if(m_selectedItem == &item || m_detailPanel.isVisible())
		m_detailPanel.display(item);
	m_selectedBlock = nullptr;
	m_selectedItem = &item;
	m_selectedPlant = nullptr;
	m_selectedActor = nullptr;
}
void Window::selectPlant(Plant& plant)
{
	if(m_selectedPlant == &plant || m_detailPanel.isVisible())
		m_detailPanel.display(plant);
	m_selectedPlant = nullptr;
	m_selectedItem = nullptr;
	m_selectedPlant = &plant;
	m_selectedActor = nullptr;
}
void Window::selectActor(Actor& actor)
{
	if(m_selectedActor == &actor || m_detailPanel.isVisible())
		m_detailPanel.display(actor);
	m_selectedBlock = nullptr;
	m_selectedItem = nullptr;
	m_selectedPlant = nullptr;
	m_selectedActor = &actor;
}
[[nodiscard]] Json Window::povToJson() const
{
	Json data{
		{"area", m_area},
		{"scale", m_scale},
		{"m_z", m_z}
	};
	if(m_selectedBlock)
	{
		data["blockX"] = m_selectedBlock->m_x;
		data["blockY"] = m_selectedBlock->m_y;
	}
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
