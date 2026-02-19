set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
file(GLOB UI_FILES CONFIGURE_DEPENDS *.cpp)
file(GLOB IMGUI_FILES CONFIGURE_DEPENDS imgui/*.cpp)
file(GLOB SCREEN_FILES CONFIGURE_DEPENDS screens/*.cpp)
add_executable (UI ${UI_FILES} ${IMGUI_FILES} ${SCREEN_FILES})
target_compile_features(UI PUBLIC cxx_std_23)
target_link_libraries(UI LINK_PUBLIC Engine)
target_link_libraries(UI PRIVATE SDL2::SDL2)
target_link_libraries(UI PRIVATE OpenGL::GL)
add_custom_command(
	TARGET UI POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E rename
	${CMAKE_CURRENT_BINARY_DIR}/UI
	${CMAKE_SOURCE_DIR}/UI
)
