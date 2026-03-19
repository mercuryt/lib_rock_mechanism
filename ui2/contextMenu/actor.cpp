#include "contextMenu.h"
#include "../window.h"
#include "../displayData.h"
#include "../widgets/widgets.h"
#include "../../engine/actors/actors.h"
#include "../../engine/objectives/kill.h"
#include "../../engine/objectives/goTo.h"
#include "../../engine/objectives/station.h"
#include "../../engine/space/space.h"
#include "../../engine/area/area.h"
#include "../../engine/simulation/hasActors.h"
#include "../../engine/definitions/animalSpecies.h"
void contextMenu::controlls::actors(Window& window)
{
	if(ImGui::BeginMenu("actors"))
	{
		Area& area = *window.m_area;
		Space& space =  area.getSpace();
		Actors& actors = area.getActors();
		ControllsState& state = window.m_gameOverlay.m_controllsState;
		SmallSet<ActorIndex> selected;
		if(window.m_gameOverlay.m_selectMode == SelectMode::Space)
			selected = space.actor_getAll(window.m_gameOverlay.m_selectedArea);
		else if(window.m_gameOverlay.m_selectMode == SelectMode::Actors)
			for(const ActorReference& ref : window.m_gameOverlay.m_selectedActors)
				selected.insert(ref.getIndex(actors.m_referenceData));
		else
			selected = space.actor_getAll(window.m_gameOverlay.m_blockUnderCursor);
		// Actor submenu.
		for(const ActorIndex actor : space.actor_getAll(state.clickedOnPoint))
		{
			if(ImGui::BeginMenu(actors.getName(actor).c_str()))
			{
				if(ImGui::MenuItem("info"))
				{
					window.m_gameOverlay.m_detailActor = actors.getReference(actor);
					window.m_gameOverlay.m_infoPopUp = InfoPopUpId::Actor;
					ImGui::CloseCurrentPopup();
				}
				if(ImGui::MenuItem("details"))
				{
					window.m_gameOverlay.m_detailActor = actors.getReference(actor);
					window.m_panel = PanelId::ActorDetails;
					ImGui::CloseCurrentPopup();
				}
				if(window.m_editMode)
				{
					if(ImGui::MenuItem("edit"))
					{
						window.m_gameOverlay.m_detailActor = actors.getReference(actor);
						window.m_panel = PanelId::EditActor;
						ImGui::CloseCurrentPopup();
					}

				}
				if(window.m_editMode || (window.m_faction.exists() && window.m_faction == actors.getFaction(actor)))
				{
					if(ImGui::MenuItem("priorities"))
					{
						window.m_gameOverlay.m_detailActor = actors.getReference(actor);
						window.m_panel = PanelId::ObjectivePriorities;
						ImGui::CloseCurrentPopup();
					}
				}
				if(window.m_editMode)
				{
					if(ImGui::MenuItem("destroy"))
					{
						window.m_gameOverlay.deselectAll();
						actors.destroy(actor);
						const ActorId& actorId = actors.getId(actor);
						window.m_simulation->m_actors.removeActor(actorId);
						ImGui::CloseCurrentPopup();
					}
				}
				if(!selected.empty() &&
					(selected.size() != 1 || actors.getLocation(selected.front()) != state.clickedOnPoint) &&
					(window.m_editMode || window.m_faction != actors.getFaction(actor)))
				{
					if(ImGui::MenuItem("kill"))
					{

						const ActorReference& ref = actors.getReference(actor);
						for(const ActorIndex selectedActor : selected)
							if(selectedActor != actor)
							{
								std::unique_ptr<Objective> objective = std::make_unique<KillObjective>(ref);
								actors.objective_replaceTasks(selectedActor, std::move(objective));
							}
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::EndMenu();
			}
		}
		if(!selected.empty() && (selected.size() != 1 || actors.getLocation(selected.front()) != state.clickedOnPoint))
		{
			if(space.shape_canEnterCurrentlyWithAnyFacing(state.clickedOnPoint, actors.getShape(selected.front()), actors.getOccupied(selected.front())))
			{
				if(ImGui::MenuItem("go to"))
				{
					for(const ActorIndex selectedActor : selected)
					{
						std::unique_ptr<Objective> objective = std::make_unique<GoToObjective>(state.clickedOnPoint);
						actors.objective_replaceTasks(selectedActor, std::move(objective));
					}
					ImGui::CloseCurrentPopup();
				}
				if(ImGui::MenuItem("station"))
				{
					for(const ActorIndex selectedActor : selected)
					{
						std::unique_ptr<Objective> objective = std::make_unique<StationObjective>(state.clickedOnPoint);
						actors.objective_replaceTasks(selectedActor, std::move(objective));
					}
					ImGui::CloseCurrentPopup();
				}
			}
			// TODO: targeted hauling and installing.
		}
		if(window.m_editMode)
		{
			if(ImGui::BeginMenu("create"))
			{
				ImGui::InputText("name", &state.name);
				widgets::animalSpecies(&state.animalSpecies);
				widgets::faction(&state.faction, *window.m_simulation);
				if(ImGui::MenuItem("confirm"))
				{
					const ActorIndex newActor = actors.create({
						.species = state.animalSpecies,
						.name = state.name,
						.location = state.clickedOnPoint,
						.faction = state.faction
					});
					actors.objective_maybeDoNext(newActor);
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndMenu();
			}
		}
		ImGui::EndMenu();
	}
}
