set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_executable (UI main.cpp actorPanel.cpp contextMenu.cpp infoPopup.cpp dialogueBox.cpp displayData.cpp draw.cpp editActorPanel.cpp editAreaPanel.cpp editFactionPanel.cpp editRealityPanel.cpp editStockPilePanel.cpp editDramaPanel.cpp gameOverlay.cpp load.cpp main.cpp mainMenu.cpp objectivePriorityPanel.cpp production.cpp sprite.cpp stocksPanel.cpp uiUtil.cpp uniformPanel.cpp widgets.cpp window.cpp contextMenu/actor.cpp contextMenu/craft.cpp contextMenu/dig.cpp contextMenu/fluid.cpp contextMenu/plant.cpp contextMenu/construct.cpp contextMenu/farmField.cpp contextMenu/item.cpp contextMenu/stockpile.cpp contextMenu/woodCutting.cpp widgets/item.cpp)
#worldParamatersPanel.cpp)
target_compile_features(UI PUBLIC cxx_std_23)
#target_include_directories (UI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(UI LINK_PUBLIC Engine)
find_package(TGUI 1 REQUIRED)
target_link_libraries(UI PRIVATE TGUI::TGUI)
add_custom_command(
	TARGET UI POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E rename
	${CMAKE_SOURCE_DIR}/build/ui/UI
	${CMAKE_SOURCE_DIR}/UI
)
