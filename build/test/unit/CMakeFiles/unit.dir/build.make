# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 3.22

# Delete rule output on recipe failure.
.DELETE_ON_ERROR:

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canonical targets will work.
.SUFFIXES:

# Disable VCS-based implicit rules.
% : %,v

# Disable VCS-based implicit rules.
% : RCS/%

# Disable VCS-based implicit rules.
% : RCS/%,v

# Disable VCS-based implicit rules.
% : SCCS/s.%

# Disable VCS-based implicit rules.
% : s.%

.SUFFIXES: .hpux_make_needs_suffix_list

# Command-line flag to silence nested $(MAKE).
$(VERBOSE)MAKESILENT = -s

#Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E rm -f

# Escaping for special characters.
EQUALS = =

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/mercury/Code/lib_rock_mechanism

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/mercury/Code/lib_rock_mechanism/build

# Include any dependencies generated for this target.
include test/unit/CMakeFiles/unit.dir/depend.make
# Include any dependencies generated by the compiler for this target.
include test/unit/CMakeFiles/unit.dir/compiler_depend.make

# Include the progress variables for this target.
include test/unit/CMakeFiles/unit.dir/progress.make

# Include the compile flags for this target's objects.
include test/unit/CMakeFiles/unit.dir/flags.make

test/unit/CMakeFiles/unit.dir/test.cpp.o: test/unit/CMakeFiles/unit.dir/flags.make
test/unit/CMakeFiles/unit.dir/test.cpp.o: ../test/unit/test.cpp
test/unit/CMakeFiles/unit.dir/test.cpp.o: test/unit/CMakeFiles/unit.dir/compiler_depend.ts
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --progress-dir=/home/mercury/Code/lib_rock_mechanism/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_1) "Building CXX object test/unit/CMakeFiles/unit.dir/test.cpp.o"
	cd /home/mercury/Code/lib_rock_mechanism/build/test/unit && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -MD -MT test/unit/CMakeFiles/unit.dir/test.cpp.o -MF CMakeFiles/unit.dir/test.cpp.o.d -o CMakeFiles/unit.dir/test.cpp.o -c /home/mercury/Code/lib_rock_mechanism/test/unit/test.cpp

test/unit/CMakeFiles/unit.dir/test.cpp.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/unit.dir/test.cpp.i"
	cd /home/mercury/Code/lib_rock_mechanism/build/test/unit && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -E /home/mercury/Code/lib_rock_mechanism/test/unit/test.cpp > CMakeFiles/unit.dir/test.cpp.i

test/unit/CMakeFiles/unit.dir/test.cpp.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/unit.dir/test.cpp.s"
	cd /home/mercury/Code/lib_rock_mechanism/build/test/unit && /usr/bin/c++ $(CXX_DEFINES) $(CXX_INCLUDES) $(CXX_FLAGS) -S /home/mercury/Code/lib_rock_mechanism/test/unit/test.cpp -o CMakeFiles/unit.dir/test.cpp.s

# Object files for target unit
unit_OBJECTS = \
"CMakeFiles/unit.dir/test.cpp.o"

# External object files for target unit
unit_EXTERNAL_OBJECTS =

test/unit/unit: test/unit/CMakeFiles/unit.dir/test.cpp.o
test/unit/unit: test/unit/CMakeFiles/unit.dir/build.make
test/unit/unit: src/libSource.a
test/unit/unit: test/unit/CMakeFiles/unit.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green --bold --progress-dir=/home/mercury/Code/lib_rock_mechanism/build/CMakeFiles --progress-num=$(CMAKE_PROGRESS_2) "Linking CXX executable unit"
	cd /home/mercury/Code/lib_rock_mechanism/build/test/unit && $(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/unit.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
test/unit/CMakeFiles/unit.dir/build: test/unit/unit
.PHONY : test/unit/CMakeFiles/unit.dir/build

test/unit/CMakeFiles/unit.dir/clean:
	cd /home/mercury/Code/lib_rock_mechanism/build/test/unit && $(CMAKE_COMMAND) -P CMakeFiles/unit.dir/cmake_clean.cmake
.PHONY : test/unit/CMakeFiles/unit.dir/clean

test/unit/CMakeFiles/unit.dir/depend:
	cd /home/mercury/Code/lib_rock_mechanism/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/mercury/Code/lib_rock_mechanism /home/mercury/Code/lib_rock_mechanism/test/unit /home/mercury/Code/lib_rock_mechanism/build /home/mercury/Code/lib_rock_mechanism/build/test/unit /home/mercury/Code/lib_rock_mechanism/build/test/unit/CMakeFiles/unit.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : test/unit/CMakeFiles/unit.dir/depend

