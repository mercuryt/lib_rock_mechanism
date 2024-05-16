#include "../widgets.h"
#include "../engine/item.h"
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
		const ItemType& itemType = *lastSelectedItemType;
		for(const MaterialType* materialType : sortByName(materialTypeDataStore))
		{
			if(!itemType.materialTypeCategories.empty())
			{
				for(const MaterialTypeCategory* materialTypeCategory : itemType.materialTypeCategories)
					if(materialTypeCategory == materialType->materialTypeCategory)
					{
						materialTypeUI->addItem(materialType->name, materialType->name);
						break;
					}
				continue;
			}
			materialTypeUI->addItem(materialType->name, materialType->name);
		}
		if(lastSelectedMaterial && materialTypeUI->contains(lastSelectedMaterial->name))
			materialTypeUI->setSelectedItem(lastSelectedMaterial->name);
		else
			materialTypeUI->setSelectedItemByIndex(0);
		wearUI->setVisible(!itemType.generic);
		wearLabel->setVisible(!itemType.generic);
		quantityOrQualityLabel->setText(itemType.generic ? "quantity" : "quality");
		if(itemType.generic && !quantityOrQuality)
			quantityOrQuality = 1;
	};
	itemTypeUI->onItemSelect([populate](const tgui::String itemType){ 
		if(itemType.empty())
			return;
		lastSelectedItemType = &ItemType::byName(itemType.toStdString());
		populate();
	});
	materialTypeUI->onItemSelect([](const tgui::String name){ 
		if(name.empty())
			return;
		lastSelectedMaterial = &MaterialType::byName(name.toStdString());
	});
	for(const ItemType* itemType : sortByName(itemTypeDataStore))
		itemTypeUI->addItem(itemType->name, itemType->name);
	// Populate is called when item type is set.
	if(lastSelectedItemType)
		itemTypeUI->setSelectedItemById(lastSelectedItemType->name);
	else
		itemTypeUI->setSelectedItemByIndex(0);
	confirmUI->onClick([quantityOrQualityUI, wearUI, callback]{
		ItemParamaters params{
			.itemType=*lastSelectedItemType,
			.materialType=*lastSelectedMaterial
		};
		if(params.itemType.generic)
			params.quantity=quantityOrQualityUI->getValue();
		else
		{
			params.quality=quantityOrQualityUI->getValue();
			params.percentWear=wearUI->getValue();
		}
		callback(params);
	});
	return {itemTypeUI, materialTypeUI, quantityOrQualityLabel, quantityOrQualityUI, wearLabel, wearUI, confirmUI};
}
