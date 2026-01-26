#pragma once
#include "../engine/uniform.h"
#include <TGUI/TGUI.hpp>
#include <TGUI/Widgets/EditBox.hpp>
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <bits/ranges_algo.h>
class Window;

class UniformCreateOrEditView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Label::Ptr m_title;
	tgui::EditBox::Ptr m_name;
	tgui::Grid::Ptr m_elements;
	tgui::Button::Ptr m_cancel;
	tgui::Button::Ptr m_confirm;
	Uniform* m_uniform;
	Uniform m_copy;
	std::string displayNameForElement(UniformElement& uniformElement);
	// What does this do?
	void setElementQuantity(UniformElement& uniformElement, int quantity);
	void removeElement(UniformElement& uniformElement);
public:
	UniformCreateOrEditView(Window& window);
	void show() { m_group->setVisible(true); }
	void hide() { m_group->setVisible(false); }
	void draw(Uniform* uniform);
	void drawElements();
	void save();
	void cancel();
};
class UniformListView final
{
	Window& m_window;
	tgui::Group::Ptr m_group;
	tgui::Label::Ptr m_name;
	tgui::ScrollablePanel::Ptr m_list;
	tgui::Button::Ptr m_create;
	tgui::Button::Ptr m_close;
	UniformCreateOrEditView m_uniformCreateOrEditView;
public:
	UniformListView(Window& w);
	void show() { m_group->setVisible(true); m_uniformCreateOrEditView.hide(); }
	void hide() { m_group->setVisible(false); m_uniformCreateOrEditView.hide(); }
	void create();
	void edit(Uniform& uniform);
	void draw();
};
