#include "../contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../../engine/actors/actors.h"
#include "../../engine/objectives/kill.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/station.h"
#include "../../engine/space/space.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/hasShapes.hpp"
void ContextMenu::drawActorControls(const Point3D& point)
{
	Area& area = *m_window.getArea();
	Space& space =  area.getSpace();
	Actors& actors = area.getActors();
	// Actor submenu.
	for(const ActorIndex& actor : space.actor_getAll(point))
	{
		auto label = tgui::Label::create(actors.getName(actor));
		label->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(label);
		label->onMouseEnter([this, actor, &actors, &area]{
			auto& submenu = makeSubmenu(0);
			auto actorInfoButton = tgui::Button::create("info");
			submenu.add(actorInfoButton);
			actorInfoButton->onClick([this, actor]{ m_window.getGameOverlay().drawInfoPopup(actor);});
			auto actorDetailButton = tgui::Button::create("details");
			submenu.add(actorDetailButton);
			actorDetailButton->onClick([this, actor]{ m_window.showActorDetail(actor); hide(); });
			if(m_window.m_editMode)
			{
				auto actorEditButton = tgui::Button::create("edit");
				submenu.add(actorEditButton);
				actorEditButton->onClick([this, actor]{ m_window.showEditActor(actor); hide(); });
			}
			if(m_window.m_editMode || (m_window.getFaction().exists() && m_window.getFaction() == actors.getFaction(actor)))
			{
				auto priorities = tgui::Button::create("priorities");
				priorities->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
				submenu.add(priorities);
				priorities->onClick([this, actor]{
					m_window.showObjectivePriority(actor);
					hide();
				});
			}
			if(m_window.m_editMode)
			{
				auto destroy = tgui::Button::create("destroy");
				destroy->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
				destroy->onClick([this, actor, &actors, &area]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					m_window.deselectAll();
					actors.exit(actor);
					actors.destroy(actor);
					const ActorId& actorId = actors.getId(actor);
					m_window.getSimulation()->m_actors.removeActor(actorId);
					hide();
				});
				submenu.add(destroy);
			}
			auto& selectedActors = m_window.getSelectedActors();
			if(!selectedActors.empty() && (m_window.m_editMode || m_window.getFaction() != actors.getFaction(actor)))
			{
				tgui::Button::Ptr kill = tgui::Button::create("kill");
				submenu.add(kill);
				kill->onClick([this, actor, &actors, &selectedActors]{
					std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
					const ActorReference& ref = actors.getReference(actor);
					for(const ActorIndex& selectedActor : selectedActors)
					{
						std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(ref);
						actors.objective_replaceTasks(selectedActor, std::move(objective));
					}
					hide();
				});
			}
		});
	}
	auto& selectedActors = m_window.getSelectedActors();
	if(selectedActors.size())
	{
		// Go.
		auto goTo = tgui::Button::create("go to");
		goTo->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(goTo);
		goTo->onClick([this, point, &actors]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(const ActorIndex& actor : m_window.getSelectedActors())
			{
				std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(point);
				actors.objective_replaceTasks(actor, std::move(objective));
			}
			hide();
		});
		// Station.
		auto station = tgui::Button::create("station");
		station->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(station);
		station->onClick([this, point, &actors]{
			std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
			for(const ActorIndex& actor : m_window.getSelectedActors())
			{
				std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(point);
				actors.objective_replaceTasks(actor, std::move(objective));
			}
			hide();
		});
	}
	if(m_window.m_editMode)
	{
		auto createActor = tgui::Label::create("create actor");
		createActor->getRenderer()->setBackgroundColor(displayData::contextMenuHoverableColor);
		m_root.add(createActor);
		createActor->onMouseEnter([this, point, &actors]{
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
			confirm->onClick([this, point, nameUI, speciesUI, factionUI, &actors]{
				std::lock_guard lock(m_window.getSimulation()->m_uiReadMutex);
				const ActorIndex& actor = actors.create({
					.species = widgetUtil::lastSelectedAnimalSpecies,
					.name = nameUI->getText().toWideString(),
					.location = point,
					.faction = widgetUtil::lastSelectedFaction
				});
				actors.objective_maybeDoNext(actor);
				hide();
			});
		});
	}
}
