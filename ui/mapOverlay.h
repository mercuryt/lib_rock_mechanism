#pragma once
#include <TGUI/TGUI.hpp>
class Window;
struct WorldLocation;
enum class ReasonForShowingMap{ PickStartingPoint, View };
class MapOverlay final
{
public:
	MapOverlay(Window& w, ReasonForShowingMap reasonForShowingMap);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void drawDetailPanel(const WorldLocation& location) { m_detailPanel->draw(location); }
	void closeDetailPanel() { m_detailPanel->setVisible(false); }
	[[nodiscard]] bool isVisible() const { return m_group->isVisible(); }
	[[nodiscard]] bool menuIsVisible() const { return m_detailPanel->isVisible(); }
private:
	
	Window& m_window;
	tgui::Group::Ptr m_group;
	MapDetailPanel m_detailPanel;
	ReasonForShowingMap m_reasonForShowingMap;
};
