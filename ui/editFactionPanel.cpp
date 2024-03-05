#include "editFactionPanel.h"
#include "../engine/faction.h"
#include "widgets.h"
#include "../engine/simulation.h"
#include "window.h"
#include <TGUI/Layout.hpp>
#include <TGUI/Widget.hpp>

EditFactionView::EditFactionView(Window& window) : m_window(window), m_panel(tgui::Panel::create())
{ 
	m_panel->setVisible(false);
	m_window.getGui().add(m_panel);
}
void EditFactionView::draw(Faction* faction)
{
	m_panel->removeAllWidgets();
	auto label = tgui::Label::create(faction ? "edit faction" : "create faction");
	m_panel->add(label);
	auto nameUI = tgui::EditBox::create();
	nameUI->setPosition(10, tgui::bindBottom(label) + 10);
	if(faction)
		nameUI->setText(faction->name);
	m_panel->add(nameUI);
	auto save = tgui::Button::create("save");
	m_panel->add(save);
	save->onClick([this, nameUI, faction] () mutable {
		auto name = nameUI->getText().toWideString();
		assert(!name.empty());
		if(faction)
		{
			assert(faction->name != name);
			faction->name = name;
		}
		else
			faction = &m_window.getSimulation()->createFaction(name);
		m_window.showEditFaction(faction);
	});
	save->setEnabled(false);
	nameUI->onTextChange([save, faction](tgui::String text){ 
		save->setEnabled(!text.empty() && (!faction || faction->name != text.toWideString()));
	});
	save->setPosition(tgui::bindLeft(nameUI), tgui::bindBottom(nameUI) + 10);
	auto setPlayerFaction = tgui::Button::create("set player faction");
	m_panel->add(setPlayerFaction);
	setPlayerFaction->onClick([this, faction]{ m_window.setFaction(*faction); });
	setPlayerFaction->setPosition(tgui::bindLeft(save), tgui::bindBottom(save) + 10);
	if(!faction)
		setPlayerFaction->setEnabled(false);
	auto back = tgui::Button::create("back");
	back->onClick([this]{ m_window.showEditFactions(); });
	back->setPosition(tgui::bindLeft(setPlayerFaction), tgui::bindBottom(setPlayerFaction) + 10);
	m_panel->add(back);
	if(faction)
	{
		auto addEnemyLabel = tgui::Label::create("add enemy faction");
		m_panel->add(addEnemyLabel);
		addEnemyLabel->setPosition(tgui::bindRight(nameUI) + 10, tgui::bindTop(nameUI));
		auto addEnemyUI = widgetUtil::makeFactionSelectUI(*m_window.getSimulation(), "select");
		addEnemyUI->setPosition(tgui::bindLeft(addEnemyLabel), tgui::bindBottom(addEnemyLabel) + 10);
		addEnemyUI->onItemSelect([this, faction](tgui::String id){
			if(id != "__null")
			{
				Faction& enemy = m_window.getSimulation()->m_hasFactions.byName(id.toWideString());
				faction->enemies.insert(&enemy);
				m_window.showEditFaction(faction);
			}
		});
		m_panel->add(addEnemyUI);
		tgui::Widget::Ptr previous = addEnemyUI;
		for(Faction* enemy : faction->enemies)
		{
			auto label = tgui::Label::create(enemy->name);
			label->setPosition(tgui::bindLeft(previous), tgui::bindBottom(previous) + 10);
			m_panel->add(label);
			previous = label;
			if(m_window.m_editMode)
			{
				auto button = tgui::Button::create("X");
				button->setPosition(tgui::bindLeft(label) + 10, tgui::bindTop(label));
				m_panel->add(button);
				button->onClick([this, faction, enemy]{
					faction->enemies.erase(enemy);
					m_window.showEditFaction(faction);
				});
			}
		}

		auto addAllyLabel = tgui::Label::create("add ally faction");
		m_panel->add(addAllyLabel);
		addAllyLabel->setPosition(tgui::bindRight(addEnemyLabel) + 10, tgui::bindTop(nameUI));
		auto addAllyUI = widgetUtil::makeFactionSelectUI(*m_window.getSimulation(), "select");
		addAllyUI->setPosition(tgui::bindLeft(addAllyLabel), tgui::bindBottom(addAllyLabel) + 10);
		addAllyUI->onItemSelect([this, faction](tgui::String id){
			if(id != "__null")
			{
				Faction& ally = m_window.getSimulation()->m_hasFactions.byName(id.toWideString());
				faction->allies.insert(&ally);
				m_window.showEditFaction(faction);
			}
		});
		m_panel->add(addAllyUI);
		for(Faction* ally : faction->allies)
		{
			auto label = tgui::Label::create(ally->name);
			label->setPosition(tgui::bindLeft(previous), tgui::bindBottom(previous) + 10);
			m_panel->add(label);
			previous = label;
			if(m_window.m_editMode)
			{
				auto button = tgui::Button::create("X");
				button->setPosition(tgui::bindLeft(label) + 10, tgui::bindTop(label));
				m_panel->add(button);
				button->onClick([this, faction, ally]{
					faction->allies.erase(ally);
					m_window.showEditFaction(faction);
				});
			}
		}
	}
	m_panel->setVisible(true);
}
EditFactionsView::EditFactionsView(Window& window) : m_window(window), m_panel(tgui::Panel::create())
{
	m_window.getGui().add(m_panel);
	m_panel->setVisible(false);
}

void EditFactionsView::draw()
{
	m_panel->removeAllWidgets();
	auto label = tgui::Label::create("edit factions");
	m_panel->add(label);
	auto create = tgui::Button::create("create");
	m_panel->add(create);
	create->onClick([this]{ m_window.showEditFaction(); });
	create->setPosition(tgui::bindLeft(label), tgui::bindBottom(label) + 10);
	auto back = tgui::Button::create("back");
	m_panel->add(back);
	back->onClick([this]{ m_window.showGame(); });
	back->setPosition(tgui::bindLeft(create), tgui::bindBottom(create) + 10);
	tgui::Widget::Ptr previous = back;
	for(Faction& faction : m_window.getSimulation()->m_hasFactions.getAll())
	{
		auto button = tgui::Button::create(faction.name);
		button->setPosition(tgui::bindLeft(previous), tgui::bindBottom(previous) + 10);
		m_panel->add(button);
		button->onClick([this, &faction]{ m_window.showEditFaction(&faction); });
		previous = button;
		//TODO: destroy faction?
	}
	m_panel->setVisible(true);
}
