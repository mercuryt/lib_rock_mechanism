add_executable (unit test.cpp actor.cpp area.cpp basicNeeds.cpp buckets.cpp caveIn.cpp combat.cpp construct.cpp craft.cpp cuboid.cpp dig.cpp eventSchedule.cpp farmFields.cpp fluid.cpp getNthAdjacent.cpp haul.cpp leadAndFollow.cpp objective.cpp plant.cpp reserve.cpp route.cpp stockpile.cpp temperatureSource.cpp threadedTask.cpp uniform.cpp vision.cpp visionCuboid.cpp woodcutting.cpp wound.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_link_libraries(unit LINK_PUBLIC Engine)

set(CMAKE_BUILD_TYPE Debug)
