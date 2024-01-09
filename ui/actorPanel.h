#pragma once
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/Group.hpp>
#include <TGUI/Widgets/ScrollablePanel.hpp>
class Window;
class Actor;
class ActorView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Label::Ptr m_name;
	tgui::Grid::Ptr m_skills;
	tgui::Grid::Ptr m_equipment;
	tgui::Grid::Ptr m_wounds;
	tgui::Label::Ptr m_bleedRate;
	tgui::ComboBox::Ptr m_uniform;
	Actor* m_actor;
public:
	ActorView(Window& w);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void draw(Actor& actor);
};
