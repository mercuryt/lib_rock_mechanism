add_executable (unit test.cpp actor.cpp attributes.cpp area.cpp basicNeeds.cpp buckets.cpp caveIn.cpp combat.cpp construct.cpp craft.cpp cuboid.cpp dig.cpp eventSchedule.cpp farmFields.cpp fluid.cpp getNthAdjacent.cpp json.cpp haul.cpp item.cpp leadAndFollow.cpp objective.cpp plant.cpp reserve.cpp route.cpp stockpile.cpp temperatureSource.cpp threadedTask.cpp uniform.cpp vision.cpp visionCuboid.cpp weather.cpp woodcutting.cpp wound.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_link_libraries(unit LINK_PUBLIC Engine)
add_custom_command(
	TARGET unit POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
	${CMAKE_SOURCE_DIR}/build/test/unit/unit
	${CMAKE_SOURCE_DIR}/unit
)
