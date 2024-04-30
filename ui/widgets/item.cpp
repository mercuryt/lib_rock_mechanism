#include "../widgets.h"
#include "item.h"
#include "materialType.h"
#include <TGUI/Renderers/ComboBoxRenderer.hpp>
#include <TGUI/Widgets/SpinButton.hpp>
#include <TGUI/Widgets/SpinControl.hpp>
#include <chrono>

tgui::Grid::Ptr widgetUtil::makeCreateItemUI(std::function<void(const ItemType&, const MaterialType&, uint32_t, uint32_t)> callback)
{
	tgui::Grid::Ptr output = tgui::Grid::create();
	tgui::ComboBox::Ptr itemTypeUI = tgui::ComboBox::create();
	tgui::ComboBox::Ptr materialTypeUI = tgui::ComboBox::create();
	tgui::Label::Ptr quantityOrQualityLabel = tgui::Label::create();
	tgui::SpinControl::Ptr quantityOrQualityUI = tgui::SpinControl::create();
	quantityOrQualityUI->setMinimum(0);
	quantityOrQualityUI->setMaximum(UINT32_MAX);
	tgui::Label::Ptr wearLabel = tgui::Label::create();
	tgui::SpinControl::Ptr wearUI = tgui::SpinControl::create();
	wearUI->setMinimum(0);
	wearUI->setMaximum(UINT32_MAX);
	tgui::Button::Ptr confirmUI = tgui::Button::create("confirm");
	output->addWidget(itemTypeUI, 0, 0);
	output->addWidget(materialTypeUI, 1, 0);
	output->addWidget(quantityOrQualityLabel, 2, 0);
	output->addWidget(quantityOrQualityUI, 2, 1);
	output->addWidget(wearLabel, 3, 0);
	output->addWidget(wearUI, 3, 1);
	output->addWidget(confirmUI, 3, 0);
	auto populate = [materialTypeUI, quantityOrQualityUI, quantityOrQualityLabel, wearUI, wearLabel]{
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
				break;
			}
			materialTypeUI->addItem(materialType->name, materialType->name);
		}
		wearUI->setVisible(itemType.generic);
		wearLabel->setVisible(itemType.generic);
		quantityOrQualityLabel->setText(itemType.generic ? "quantity" : "quality");
	};
	confirmUI->onClick([quantityOrQualityUI, wearUI, callback]{
		const ItemType& itemType = *lastSelectedItemType;
		const MaterialType& materialType = *lastSelectedMaterial;
		callback(itemType, materialType, quantityOrQualityUI->getValue(), wearUI->getValue());
	});
	return output;
}
