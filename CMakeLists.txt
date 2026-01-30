cmake_minimum_required(VERSION 3.22.1)

set(CMAKE_CXX_COMPILER g++)

project(LibRockMechanism)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_CXX_FLAGS "-fuse-ld=mold")
set(CMAKE_CXX_COMPILER_LAUNCHER ccache)

if(MSVC)
	# warning level 4
	add_compile_options(/W4)
else()
	# additional warnings
	add_compile_options(-Wall -Wextra -Werror -Wshadow -Wunused-function -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wdouble-promotion -Woverloaded-virtual -fopenmp -march=native
	#-ftrivial-auto-var-init=pattern
	# Disabled due to false positives around const ObjectiveType& types
	-Wno-dangling-reference
	)
	# Causes wierd errors...
	#-Wnull-dereference -D_GLIBCXX_SANITIZE_VECTOR
	# Incompatable with tgui?
	#add_compile_options(-D_GLIBCXX_DEBUG)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g2 -ggdb -fno-inline")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Ofast -flto=auto -ltcmalloc -fno-rtti")
if(NOT CMAKE_BUILD_TYPE EQUAL "Release")
	#add_compile_options(-fsanitize=address)
	#add_link_options(-fsanitize=address)
	#add_compile_options(-fsanitize=undefined)
	#add_link_options(-fsanitize=undefined)
endif()

add_subdirectory(engine)
#add_subdirectory(ui)
add_subdirectory(test)
#add_subdirectory(python)
