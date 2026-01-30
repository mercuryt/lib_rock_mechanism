add_executable (unit test.cpp bitset.cpp cuboidSet.cpp geometry.cpp rtree.cpp actor.cpp attributes.cpp area.cpp basicNeeds.cpp space.cpp buckets.cpp caveIn.cpp combat.cpp construct.cpp craft.cpp cuboid.cpp dig.cpp eventSchedule.cpp farmFields.cpp fluid.cpp getNthAdjacent.cpp json.cpp haul.cpp item.cpp leadAndFollow.cpp objective.cpp octTree.cpp plant.cpp reference.cpp reserve.cpp route.cpp stockpile.cpp temperatureSource.cpp threadedTask.cpp uniform.cpp vision.cpp weather.cpp woodcutting.cpp wound.cpp physics.cpp mount.cpp vehicle.cpp)

target_link_libraries(unit LINK_PUBLIC Engine)
add_custom_command(
	TARGET unit POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E rename
	${CMAKE_CURRENT_BINARY_DIR}/unit
	${CMAKE_SOURCE_DIR}/unit
	COMMAND notify-send done
)
