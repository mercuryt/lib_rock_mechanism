#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/actor.h"
#include "../../engine/objectives/kill.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/station.h"
void ContextMenu::drawActorControls(Block& block)
{
	// Actor submenu.
	for(Actor* actor : block.m_hasActors.getAll())
	{
		auto label = tgui::Label::create(actor->m_name);
		label->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, actor]{
			auto& submenu = makeSubmenu(0);
			auto actorInfoButton = tgui::Button::create("info");
			submenu.add(actorInfoButton);
			actorInfoButton->onClick([this, actor]{ m_window.getGameOverlay().drawInfoPopup(*actor);});
			auto actorDetailButton = tgui::Button::create("details");
			submenu.add(actorDetailButton);
			actorDetailButton->onClick([this, actor]{ m_window.showActorDetail(*actor); hide(); });
			if(m_window.m_editMode)
			{
				auto actorEditButton = tgui::Button::create("edit");
				submenu.add(actorEditButton);
				actorEditButton->onClick([this, actor]{ m_window.showEditActor(*actor); hide(); });
			}
			if(m_window.m_editMode || (m_window.getFaction() && m_window.getFaction() == actor->getFaction()))
			{
				auto priorities = tgui::Button::create("priorities");
				priorities->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
				submenu.add(priorities);
				priorities->onClick([this, actor]{
					m_window.showObjectivePriority(*actor);
					hide();
				});
			}
			if(m_window.m_editMode)
			{
				auto destroy = tgui::Button::create("destroy");
				destroy->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
				destroy->onClick([this, actor]{ 
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					m_window.deselectAll();
					m_window.getSimulation()->destroyActor(*actor);
					hide();
				});
				submenu.add(destroy);
			}
			auto& actors = m_window.getSelectedActors();
			if(!actors.empty() && (m_window.m_editMode || m_window.getFaction() != actor->getFaction()))
			{
				tgui::Button::Ptr kill = tgui::Button::create("kill");
				submenu.add(kill);
				kill->onClick([this, actor, &actors]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					for(Actor* selectedActor : actors)
					{
						std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(*selectedActor, *actor);
						selectedActor->m_hasObjectives.replaceTasks(std::move(objective));
					}
					hide();
				});
			}
		});
	}
	auto& actors = m_window.getSelectedActors();
	if(actors.size())
	{
		// Go.
		auto goTo = tgui::Button::create("go to");
		goTo->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(goTo);
		goTo->onClick([this, &block]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(Actor* actor : m_window.getSelectedActors())
			{
				std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(*actor, block);
				actor->m_hasObjectives.replaceTasks(std::move(objective));
			}
			hide();
		});
		// Station.
		auto station = tgui::Button::create("station");
		station->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(station);
		station->onClick([this, &block]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(Actor* actor : m_window.getSelectedActors())
			{
				std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(*actor, block);
				actor->m_hasObjectives.replaceTasks(std::move(objective));
			}
			hide();
		});
	}
	if(m_window.m_editMode)
	{
		auto createActor = tgui::Label::create("create actor");
		createActor->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(createActor);
		createActor->onMouseEnter([this, &block]{
			auto& submenu = makeSubmenu(0);
			auto nameLabel = tgui::Label::create("name");
			nameLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(nameLabel);
			auto nameUI = tgui::EditBox::create();
			submenu.add(nameUI);
			auto speciesLabel = tgui::Label::create("species");
			speciesLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(speciesLabel);
			auto speciesUI = widgetUtil::makeAnimalSpeciesSelectUI();
			submenu.add(speciesUI);
			auto factionLabel = tgui::Label::create("faction");
			factionLabel->getRenderer()->setBackgroundColor(displayData::contextMenuUnhoverableColor);
			submenu.add(factionLabel);
			auto factionUI = widgetUtil::makeFactionSelectUI(*m_window.getSimulation(), L"none");
			submenu.add(factionUI);
			//TODO: generate a default name when species is selected if name is blank.
			auto confirm = tgui::Button::create("create");
			confirm->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
			submenu.add(confirm);
			confirm->onClick([this, &block, nameUI, speciesUI, factionUI]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				Actor& actor = m_window.getSimulation()->createActor(ActorParamaters{
					.species = *widgetUtil::lastSelectedAnimalSpecies,
					.name = nameUI->getText().toWideString(),
					.location = &block,
					.faction = widgetUtil::lastSelectedFaction
				});
				actor.m_hasObjectives.getNext();
				hide();
			});
		});
	}
}
