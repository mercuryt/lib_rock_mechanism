#include "../widgets.h"
#include "../engine/items/items.h"
#include "../engine/materialType.h"
#include "../displayData.h"

std::array<tgui::Widget::Ptr, 7> widgetUtil::makeCreateItemUI(std::function<void(ItemParamaters)> callback)
{
	static uint32_t quantityOrQuality = 0;
	static uint32_t wear = 0;
	tgui::ComboBox::Ptr itemTypeUI = tgui::ComboBox::create();
	itemTypeUI->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	tgui::ComboBox::Ptr materialTypeUI = tgui::ComboBox::create();
	materialTypeUI->setItemsToDisplay(displayData::maximumNumberOfItemsToDisplayInComboBox);
	tgui::Label::Ptr quantityOrQualityLabel = tgui::Label::create();
	tgui::SpinControl::Ptr quantityOrQualityUI = tgui::SpinControl::create();
	quantityOrQualityUI->setMinimum(0);
	quantityOrQualityUI->setMaximum(UINT32_MAX);
	quantityOrQualityUI->setValue(quantityOrQuality);
	quantityOrQualityUI->onValueChange([](const float value){ quantityOrQuality = value; });
	tgui::Label::Ptr wearLabel = tgui::Label::create("wear");
	tgui::SpinControl::Ptr wearUI = tgui::SpinControl::create();
	wearUI->setMinimum(0);
	wearUI->setMaximum(UINT32_MAX);
	wearUI->setValue(wear);
	wearUI->onValueChange([](const float value){ wear = value; });
	tgui::Button::Ptr confirmUI = tgui::Button::create("confirm");
	auto populate = [materialTypeUI, quantityOrQualityLabel, wearUI, wearLabel]{
		materialTypeUI->removeAllItems();
		const ItemTypeId& itemType = lastSelectedItemType;
		auto typeEnd = MaterialType::size();
		for(auto materialType = MaterialTypeId::create(0); materialType < typeEnd; ++materialType)
		{
			auto& name = MaterialType::getName(materialType);
			if(!ItemType::getMaterialTypeCategories(itemType).empty())
			{
				for(const MaterialCategoryTypeId& materialTypeCategory : ItemType::getMaterialTypeCategories(itemType))
					if(materialTypeCategory == MaterialType::getMaterialTypeCategory(materialType))
					{
						materialTypeUI->addItem(name, name);
						break;
					}
				continue;
			}
			materialTypeUI->addItem(name, name);
		}
		if(lastSelectedMaterial.exists() && materialTypeUI->contains(MaterialType::getName(lastSelectedMaterial)))
			materialTypeUI->setSelectedItem(MaterialType::getName(lastSelectedMaterial));
		else
			materialTypeUI->setSelectedItemByIndex(0);
		wearUI->setVisible(!ItemType::getIsGeneric(itemType));
		wearLabel->setVisible(!ItemType::getIsGeneric(itemType));
		quantityOrQualityLabel->setText(ItemType::getIsGeneric(itemType) ? "quantity" : "quality");
		if(ItemType::getIsGeneric(itemType) && !quantityOrQuality)
			quantityOrQuality = 1;
	};
	itemTypeUI->onItemSelect([populate](const tgui::String itemType){
		if(itemType.empty())
			return;
		lastSelectedItemType = ItemType::byName(itemType.toWideString());
		populate();
	});
	materialTypeUI->onItemSelect([](const tgui::String name){
		if(name.empty())
			return;
		lastSelectedMaterial = MaterialType::byName(name.toWideString());
	});
	auto end = ItemType::size();
	for(ItemTypeId itemType = ItemTypeId::create(0); itemType < end; ++itemType)
	{
		auto& name = ItemType::getName(itemType);
		itemTypeUI->addItem(name, name);
	}
	// Populate is called when item type is set.
	if(lastSelectedItemType.exists())
		itemTypeUI->setSelectedItemById(ItemType::getName(lastSelectedItemType));
	else
		itemTypeUI->setSelectedItemByIndex(0);
	confirmUI->onClick([quantityOrQualityUI, wearUI, callback]{
		ItemParamaters params{
			.itemType=lastSelectedItemType,
			.materialType=lastSelectedMaterial
		};
		if(ItemType::getIsGeneric(params.itemType))
			params.quantity=Quantity::create(quantityOrQualityUI->getValue());
		else
		{
			params.quality=Quality::create(quantityOrQualityUI->getValue());
			params.percentWear=Percent::create(wearUI->getValue());
		}
		callback(params);
	});
	return {itemTypeUI, materialTypeUI, quantityOrQualityLabel, quantityOrQualityUI, wearLabel, wearUI, confirmUI};
}
