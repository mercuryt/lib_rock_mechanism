#include "uniformPanel.h"
#include "window.h"
#include "../engine/item.h"
#include <TGUI/Widgets/ScrollablePanel.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <unordered_set>
UniformCreateOrEditView::UniformCreateOrEditView(Window& window) : 
	m_window(window), m_group(tgui::Group::create()), m_title(tgui::Label::create()), m_name(tgui::EditBox::create()), m_elements(tgui::Grid::create()), m_cancel(tgui::Button::create()), m_confirm(tgui::Button::create()), m_uniform(nullptr)
{
	m_window.getGui().add(m_group);
	m_group->setVisible(false);
	m_group->add(m_title);
	m_group->add(m_name);
	m_group->add(m_elements);
	m_group->add(m_cancel);
	m_group->add(m_confirm);
	m_cancel->onClick([&]{ m_window.showUniforms(); });
	m_confirm->onClick([&]{
		if(m_uniform)
		{
			m_uniform->name = m_name->getText().toWideString();
			m_uniform->elements.swap(m_copy.elements);
		}
		else
			m_window.getSimulation()->m_hasUniforms.at(*m_window.getFaction()).createUniform(m_copy.name, m_copy.elements);
		m_window.showUniforms();
	});
}
void UniformCreateOrEditView::draw(Uniform* uniform)
{
	m_uniform = uniform;
	if(uniform)
	{
		m_copy = *m_uniform;
		m_title->setText("Edit Uniform");
		drawElements();
	}
	else
	{
		m_copy.name = L"";
		m_copy.elements.clear();
		m_title->setText("Create Uniform");
		m_elements->removeAllWidgets();
	}
	m_name->setText(m_copy.name);
}
void UniformCreateOrEditView::drawElements()
{
	m_elements->removeAllWidgets();
	std::vector<UniformElement*> elements;
	for(const UniformElement& element : m_copy.elements)
		elements.push_back(&const_cast<UniformElement&>(element));
	uint32_t column = 0;
	for(UniformElement* element : elements)
	{
		auto label = tgui::Label::create(displayNameForElement(*element));
		m_elements->addWidget(label, 1, column);
		auto spinControl = tgui::SpinControl::create();
		spinControl->setMinimum(0);
		spinControl->setValue(element->quantity);
		spinControl->onValueChange([&](float value){ setElementQuantity(*element, value); } );
		m_elements->addWidget(spinControl, 2, column);
		auto removeControl = tgui::Button::create("X");
		removeControl->onClick([&]{ removeElement(*element); });
		m_elements->addWidget(removeControl, 3, column);
		++column;
	}
}
std::string UniformCreateOrEditView::displayNameForElement(UniformElement& uniformElement)
{
	assert(uniformElement.itemQuery.m_itemType);
	std::string output;
	if(uniformElement.itemQuery.m_materialType)
		output.append(uniformElement.itemQuery.m_materialType->name + " ");
	else if(uniformElement.itemQuery.m_materialTypeCategory)
		output.append(uniformElement.itemQuery.m_materialTypeCategory->name + " ");
	output.append(uniformElement.itemQuery.m_itemType->name);
	return output;
}
void UniformCreateOrEditView::setElementQuantity(UniformElement& uniformElement, uint32_t quantity)
{
	assert(quantity <= uniformElement.quantity);
	if(quantity == uniformElement.quantity)
	{
		removeElement(uniformElement);
		return;
	}
	uniformElement.quantity -= quantity;
}
void UniformCreateOrEditView::removeElement(UniformElement& uniformElement)
{
	std::erase(m_copy.elements, uniformElement);
	drawElements();
}
// List.
UniformListView::UniformListView(Window& window) : m_window(window), m_group(tgui::Group::create()), m_list(tgui::ScrollablePanel::create()), m_create(tgui::Button::create("+")), m_close(tgui::Button::create("X")), m_uniformCreateOrEditView(window)
{
	window.getGui().add(m_group);
	auto title = tgui::Label::create("Uniforms");
	m_group->add(title);
	m_group->add(m_list);
	m_create->onClick([&]{ create(); });
	m_group->add(m_create);
	m_close->onClick([&]{ m_window.showGame(); });
	m_group->add(m_close);
}
void UniformListView::create()
{
	hide();
	m_uniformCreateOrEditView.draw(nullptr);
}
void UniformListView::edit(Uniform& uniform)
{
	hide();
	m_uniformCreateOrEditView.draw(&uniform);
}
void UniformListView::draw()
{
	for(auto& [name, uniform] : m_window.getSimulation()->m_hasUniforms.at(*m_window.getFaction()).getAll())
	{
		auto editButton = tgui::Button::create(name);
		m_list->add(editButton);
		editButton->onClick([&]{ edit(uniform); });
	}
	show();
}
