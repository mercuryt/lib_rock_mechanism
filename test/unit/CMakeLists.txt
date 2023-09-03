add_executable (unit test.cpp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

target_link_libraries(unit LINK_PUBLIC Source)

set(CMAKE_BUILD_TYPE Debug)
