#pragma once

// Parameters for creating a world.

#include "worldforge/lociiConfig.h"
#include "worldforge/world.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Widget.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
class Window;
class LociiUI final
{
public:
	LociiUI(std::wstring name);
	LociiConfig get() const;
	tgui::VerticalLayout::Ptr m_container;
private:
	tgui::SpinControl::Ptr m_count;
	tgui::SpinControl::Ptr m_intensityMax;
	tgui::SpinControl::Ptr m_intensityMin;
	tgui::SpinControl::Ptr m_sustainMax;
	tgui::SpinControl::Ptr m_sustainMin;
	tgui::SpinControl::Ptr m_decayMax;
	tgui::SpinControl::Ptr m_decayMin;
	tgui::SpinControl::Ptr m_exponentMax;
	tgui::SpinControl::Ptr m_exponentMin;
	tgui::SpinControl::Ptr m_epicentersMax;
	tgui::SpinControl::Ptr m_epicentersMin;
};
class WorldParamatersView final
{
	Window& m_window;
	tgui::Panel::Ptr m_panel;
	LociiUI m_elevation{"elevation"};
	LociiUI m_grassland{"grassland"};
	LociiUI m_forest{"forest"};
	LociiUI m_desert{"desert"};
	LociiUI m_swamp{"swamp"};
	tgui::SpinControl::Ptr m_equatorSize;
	tgui::SpinControl::Ptr m_areaSizeX;
	tgui::SpinControl::Ptr m_areaSizeY;
	tgui::SpinControl::Ptr m_areaSizeZ;
	tgui::SpinControl::Ptr m_averageLandHeightBlocks;
	WorldConfig get() const;
public:
	WorldParamatersView(Window& w);
	void show() { m_panel->setVisible(true); }
	void hide() { m_panel->setVisible(false); }
	void createWorld();
};
