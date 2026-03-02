#include "infoPopUp.h"
#include "window.h"
#include "../engine/area/area.h"
#include "../engine/items/items.h"
#include "../engine/actors/actors.h"
#include "../engine/space/space.h"
#include "../engine/plants.h"
#include "../engine/definitions/animalSpecies.h"
#include "../engine/definitions/plantSpecies.h"

void drawInfoPopUp::begin(const std::string& title)
{
	bool canClose = false;
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 centerPos = ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
	ImGui::SetNextWindowPos(centerPos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	ImGui::Begin("infoPopUp", &canClose, windowFlags);
	float titleWidth = ImGuiCalcTextSize(title).x;
	ImVec2 windowSize = ImGui::GetContentRegionAvail();
	ImGui::SetCursorPosX((windowSize.x - titleWidth) * 0.5);
	ImGuiText(title);
}
void drawInfoPopUp::end()
{
	ImGui::End();
}
void drawInfoPopUp::actor(Window& window)
{
	Actors& actors = window.m_area->getActors();
	Items& items = window.m_area->getItems();
	const ActorReference actorRef = window.m_gameOverlay.m_detailActor;
	const ActorIndex actor = actorRef.getIndex(actors.m_referenceData);
	begin(actors.getName(actor));
	ImGui::BeginTable("basic", 2);
	ImGuiText("species");
	ImGui::TableNextColumn();
	ImGuiText(AnimalSpecies::getName(actors.getSpecies(actor)));
	ImGui::TableNextRow();
	ImGuiText("age");
	ImGui::TableNextColumn();
	ImGuiText(actors.getAgeInYears(actor).toString());
	ImGui::TableNextRow();
	ImGuiText("action");
	ImGuiText(actors.getActionDescription(actor));
	ImGui::EndTable();
	for(const ItemReference& itemRef : actors.equipment_getAll(actor))
	{
		const ItemIndex item = itemRef.getIndex(items.m_referenceData);
		if(ImGuiButton(items.description(item)))
			window.m_gameOverlay.showInfoPopUpForItem(itemRef);
	}
	if(ImGui::Button("Priorities"))
		window.showObjectivePriorities(actorRef);
	if(!actors.sleep_isAwake(actor))
		ImGuiText("sleeping");
	else
	{
		Percent tiredPercent = actors.sleep_getPercentTired(actor);
		if(actors.sleep_isTired(actor))
			tiredPercent += 100;
		ImGuiText((tiredPercent.toString() + " % tired"));
	}
	Percent hungerPercent = actors.eat_getPercentStarved(actor);
	if(actors.eat_isHungry(actor))
		hungerPercent += 100;
	ImGuiText((hungerPercent.toString() + " % hunger"));
	Percent thirstPercent = actors.drink_getPercentDead(actor);
	if(actors.drink_isThirsty(actor))
		thirstPercent += 100;
	ImGuiText((thirstPercent.toString() + " % thirst"));
	end();
}
void drawInfoPopUp::item(Window& window)
{
	Items& items = window.m_area->getItems();
	Actors& actors = window.m_area->getActors();
	const ItemReference itemRef = window.m_gameOverlay.m_detailItem;
	const ItemIndex item = itemRef.getIndex(items.m_referenceData);
	begin(items.description(item));
	ImGui::BeginTable("basic info", 2);
	if(ItemType::getIsGeneric(items.getItemType(item)))
	{
		ImGuiText("quantity");
		ImGui::TableNextColumn();
 		ImGuiText(items.getQuantity(item).toString());
	}
	else
	{
		ImGuiText("quality");
		ImGui::TableNextColumn();
 		ImGuiText(items.getQuality(item).toString());
		ImGui::TableNextRow();
		ImGuiText("wear");
		ImGui::TableNextColumn();
 		ImGuiText(items.getWear(item).toString());
	}
	ImGui::EndTable();
	if(items.cargo_exists(item))
	{
		ImGui::BeginTable("item cargo", window.m_editMode? 2 : 1);
		for(const ItemIndex& cargoItem : items.cargo_getItems(item))
		{
			if(ImGuiButton(items.description(cargoItem)))
				window.m_gameOverlay.showInfoPopUpForItem(items.getReference(cargoItem));
			if(window.m_editMode)
			{
				ImGui::TableNextColumn();
				if(ImGui::Button("x"))
				{
					items.cargo_removeItem(item, cargoItem);
					items.destroy(cargoItem);
				}
			}
		}
		ImGui::EndTable();
		ImGui::BeginTable("actor cargo", window.m_editMode? 2 : 1);
		for(const ActorIndex& actor : items.cargo_getActors(item))
		{
			if(ImGuiButton(actors.getName(actor)))
				window.m_gameOverlay.showInfoPopUpForActor(actors.getReference(actor));
		}
	}
	end();
}
void drawInfoPopUp::point(Window& window)
{
	Items& items = window.m_area->getItems();
	Actors& actors = window.m_area->getActors();
	Point3D point = window.m_gameOverlay.m_detailPoint;
	Space& space = window.m_area->getSpace();
	begin(point.toString());
	if(space.solid_isAny(point))
		ImGuiText(("solid " + MaterialType::getName(space.solid_get(point))));
	else
	{
		auto pointFeatures = space.pointFeature_getAll(point);
		if(pointFeatures.empty())
		{
			if(point.z() != 0)
			{
				const Point3D& below = point.below();
				const MaterialTypeId& belowSolidMaterial = space.solid_get(below);
				if(belowSolidMaterial.exists())
					ImGuiText(("rough " + MaterialType::getName(belowSolidMaterial) + " floor"));
				else
					ImGuiText("empty space");
			}
		}
		else
		{
			for(const PointFeature& pointFeature : pointFeatures)
			{
				std::string name = PointFeatureType::byId(pointFeature.pointFeatureType).name;
				ImGuiText((name + " " + MaterialType::getName(pointFeature.materialType)));
			}
		}
		ImGuiText(("temperature: " + std::to_string(space.temperature_get(point).get())));
		for(const ActorIndex& actor : space.actor_getAll(point))
			if(ImGuiButton(actors.getName(actor)))
				window.m_gameOverlay.showInfoPopUpForActor(actors.getReference(actor));
		for(const ItemIndex& item : space.item_getAll(point))
			if(ImGuiButton(items.description(item)))
				window.m_gameOverlay.showInfoPopUpForItem(items.getReference(item));
		if(space.plant_exists(point))
		{
			const PlantIndex& plant = space.plant_get(point);
			const Plants& plants = window.m_area->getPlants();
			if(ImGuiButton(PlantSpecies::getName(plants.getSpecies(plant))))
				window.m_gameOverlay.showInfoPopUpPlant(point);
		}
		ImGui::BeginTable("fluid", 2);
		for(const FluidData& fluidData : space.fluid_getAll(point))
		{
			ImGuiText(FluidType::getName(fluidData.type));
			ImGuiText(fluidData.volume.toString());
		}
		ImGui::EndTable();
		ImGui::BeginTable("fluid sources", 2);
		if(window.getArea()->m_fluidSources.contains(point))
		{
			const FluidSource& source = window.getArea()->m_fluidSources.at(point);
			ImGuiText((FluidType::getName(source.fluidType) + " source"));
			ImGuiText(source.level.toString());
		}
		ImGui::EndTable();
		if(window.m_faction.exists() && space.farm_contains(point, window.m_faction))
		{
			const FarmField& field = *space.farm_get(point, window.m_faction);
			ImGuiText(((field.m_plantSpecies.exists() ? PlantSpecies::getName(field.m_plantSpecies) + " " : "") + "field"));
		}
	}
	end();
}
void drawInfoPopUp::plant(Window& window)
{
	Point3D point = window.m_gameOverlay.m_detailPoint;
	Plants& plants = window.m_area->getPlants();
	Space& space = window.m_area->getSpace();
	PlantIndex plant = space.plant_get(point);
	begin(PlantSpecies::getName(plants.getSpecies(plant)));
	ImGuiText(("percent grown: " + std::to_string(plants.getPercentGrown(plant).get())));
	ImGuiText(("percent foliage: " + std::to_string(plants.getPercentFoliage(plant).get())));
	if(plants.getVolumeFluidRequested(plant) != 0)
	{
		Percent thirstPercent = plants.getFluidEventPercentComplete(plant);
		ImGuiText((std::to_string(thirstPercent.get()) + " % thirst"));
	}
	if(plants.readyToHarvest(plant))
		ImGuiText((plants.getQuantityToHarvest(plant).toString() + " harvestable fruit"));
	end();
}