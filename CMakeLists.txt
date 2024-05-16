cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_COMPILER g++)

project(LibRockMechanism)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_FLAGS "-fuse-ld=mold")
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
#add_compile_options(-fsanitize=undefined)
#add_link_options(-fsanitize=undefined)

if(MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Werror -Wunused-function -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wdouble-promotion)
    # Causes wierd errors...
    #-Wnull-dereference 
    # Incompatable with tgui?
    #add_compile_options(-D_GLIBCXX_DEBUG)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g3 -ggdb -fno-inline -fmax-errors=5")
if(CMAKE_BUILD_TYPE EQUAL "RELEASE")
	add_compile_options(-Ofast -ffp-contract=fast -fmerge-all-consts -flto)
	add_link_options(-flto)
endif()

add_subdirectory(engine)
#add_subdirectory(ui)
add_subdirectory(test)
#add_subdirectory(python)

