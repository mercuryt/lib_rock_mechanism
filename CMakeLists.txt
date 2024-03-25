cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_COMPILER g++)

project(LibRockMechanism)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -ggdb -fno-inline -fmax-errors=5")
set(CMAKE_CXX_FLAGS "-fuse-ld=mold")
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
# add_compile_options(-fsanitize=address,undefined)
# add_link_options(-fsanitize=address,undefined)

if(MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Werror -Wunused-function -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wnull-dereference -Wdouble-promotion )
    # Incompatable with tgui?
    # add_compile_options(-D_GLIBCXX_DEBUG)
endif()

# add_compile_options(-ffast-math)

add_subdirectory(engine)
add_subdirectory(ui)
#add_subdirectory(test)
#add_subdirectory(python)

# release build by default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()
