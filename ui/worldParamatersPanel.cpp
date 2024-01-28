#include "worldParamatersPanel.h"
#include "simulation.h"
#include "window.h"
#include "worldforge/biome.h"
#include "worldforge/lociiConfig.h"
#include "worldforge/world.h"
#include <TGUI/Container.hpp>
#include <TGUI/Widgets/Group.hpp>
#include <TGUI/Widgets/HorizontalLayout.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <TGUI/Widgets/VerticalLayout.hpp>

LociiUI::LociiUI(std::string name) : m_container(tgui::VerticalLayout::create()),
	m_count(tgui::SpinControl::create()), m_intensityMax(tgui::SpinControl::create()), m_intensityMin(tgui::SpinControl::create()), m_sustainMax(tgui::SpinControl::create()), m_sustainMin(tgui::SpinControl::create()), m_decayMax(tgui::SpinControl::create()), m_decayMin(tgui::SpinControl::create()), m_exponentMax(tgui::SpinControl::create()), m_exponentMin(tgui::SpinControl::create()), m_epicentersMax(tgui::SpinControl::create()), m_epicentersMin(tgui::SpinControl::create())
{
	auto label = tgui::Label::create(name);
	label->setPosition("50%", "5%");
	label->setOrigin(0.5, 0);
	m_container->add(label);

	auto grid = tgui::Grid::create();
	m_container->add(grid);
	grid->setSize("60%", "70%");
	grid->setPosition("20%", "15%");

	auto countLabel = tgui::Label::create("count");
	grid->addWidget(countLabel, 1, 1);
	m_count = tgui::SpinControl::create();
	grid->addWidget(m_count, 1, 2);

	auto intensityLabel = tgui::Label::create("minimum and maximum intensity");
	grid->addWidget(intensityLabel, 2, 1);
	m_intensityMin = tgui::SpinControl::create();
	grid->addWidget(m_intensityMin, 2, 2);
	m_intensityMax = tgui::SpinControl::create();
	grid->addWidget(m_intensityMax, 2, 3);

	auto sustainLabel = tgui::Label::create("minimum and maximum size");
	grid->addWidget(sustainLabel, 2, 1);
	m_sustainMin = tgui::SpinControl::create();
	grid->addWidget(m_sustainMin, 2, 2);
	m_sustainMax = tgui::SpinControl::create();
	grid->addWidget(m_sustainMax, 2, 3);

	auto decayLabel = tgui::Label::create("minimum and maximum slope");
	grid->addWidget(decayLabel, 2, 1);
	m_decayMin = tgui::SpinControl::create();
	grid->addWidget(m_decayMin, 2, 2);
	m_decayMax = tgui::SpinControl::create();
	grid->addWidget(m_decayMax, 2, 3);

	auto exponentLabel = tgui::Label::create("minimum and maximum slope curvature");
	grid->addWidget(exponentLabel, 2, 1);
	m_exponentMin = tgui::SpinControl::create();
	grid->addWidget(m_exponentMin, 2, 2);
	m_exponentMax = tgui::SpinControl::create();
	grid->addWidget(m_exponentMax, 2, 3);

	auto Label = tgui::Label::create("minimum and maximum number of epicenters");
	grid->addWidget(exponentLabel, 2, 1);
	m_epicentersMin = tgui::SpinControl::create();
	grid->addWidget(m_epicentersMin, 2, 2);
	m_epicentersMax = tgui::SpinControl::create();
	grid->addWidget(m_epicentersMax, 2, 3);
}
LociiConfig LociiUI::get() const
{
	return LociiConfig
	{
		static_cast<uint32_t>(m_count->getValue()),
		m_intensityMax->getValue(),
		m_intensityMin->getValue(),
		m_sustainMax->getValue(),
		m_sustainMin->getValue(),
		m_decayMax->getValue(),
		m_decayMin->getValue(),
		m_exponentMax->getValue(),
		m_exponentMin->getValue(),
		static_cast<uint32_t>(m_epicentersMax->getValue()),
		static_cast<uint32_t>(m_epicentersMin->getValue())
	};
};
WorldParamatersView::WorldParamatersView(Window& w) : m_window(w), m_panel(tgui::Panel::create()), m_equatorSize(tgui::SpinControl::create()), m_areaSizeX(tgui::SpinControl::create()), m_areaSizeY(tgui::SpinControl::create()), m_areaSizeZ(tgui::SpinControl::create()), m_averageLandHeightBlocks(tgui::SpinControl::create())
{
	m_window.getGui().add(m_panel);

	auto title = tgui::Label::create("Create World");
	title->setPosition("50%", "5%");
	title->setOrigin(0.5, 0);
	m_panel->add(title);

	auto layout = tgui::VerticalLayout::create();
	layout->setPosition("5%", "10%");
	m_panel->add(layout);

	layout->add(m_elevation.m_container);
	layout->add(m_grassland.m_container);
	layout->add(m_forest.m_container);
	layout->add(m_desert.m_container);
	layout->add(m_swamp.m_container);

	auto grid = tgui::Grid::create();
	layout->add(grid);
	auto equatorLabel = tgui::Label::create("number of local areas on the equator");
	grid->addWidget(equatorLabel, 1, 1);
	grid->addWidget(m_equatorSize, 2, 1);

	auto areaSizeLabel = tgui::Label::create("local area size");
	grid->addWidget(areaSizeLabel, 1, 2);
	grid->addWidget(tgui::Label::create("x"), 1, 3);
	grid->addWidget(m_areaSizeX, 2, 3);
	grid->addWidget(tgui::Label::create("y"), 3, 3);
	grid->addWidget(m_areaSizeY, 4, 3);
	grid->addWidget(tgui::Label::create("z"), 4, 3);
	grid->addWidget(m_areaSizeY, 5, 3);

	auto averageLandHeightLabel = tgui::Label::create("average land height blocks");
	grid->addWidget(averageLandHeightLabel, 1, 4);
	grid->addWidget(m_averageLandHeightBlocks, 2, 4);

	auto buttonLayout = tgui::HorizontalLayout::create();
	layout->add(buttonLayout);
	auto exitButton = tgui::Button::create("cancel");
	exitButton->onClick([this]{ m_window.showMainMenu(); });
	buttonLayout->add(exitButton);
	auto createButton = tgui::Button::create("create");
	createButton->onClick([this]{ createWorld(); });
	buttonLayout->add(createButton);
}
void WorldParamatersView::createWorld()
{
	Simulation& simulation = *m_window.getSimulation();
	World world{get(), simulation};
	simulation.m_world = world;
	m_window.showMapForSelectingStartPosition();
}
WorldConfig WorldParamatersView::get() const
{
	std::unordered_map<const Biome*, LociiConfig> biomeConfig
	{
		{&Biome::byType(BiomeId::Grassland), m_grassland.get()},
		{&Biome::byType(BiomeId::Forest), m_forest.get()},
		{&Biome::byType(BiomeId::Desert), m_desert.get()},
		{&Biome::byType(BiomeId::Swamp), m_swamp.get()}
	};
	return WorldConfig
	{
		m_elevation.get(),
		biomeConfig,
		static_cast<size_t>(m_equatorSize->getValue()),
		static_cast<uint32_t>(m_areaSizeX->getValue()),
		static_cast<uint32_t>(m_areaSizeY->getValue()),
		static_cast<uint32_t>(m_areaSizeZ->getValue()),
		static_cast<size_t>(m_averageLandHeightBlocks->getValue())
	};
}
