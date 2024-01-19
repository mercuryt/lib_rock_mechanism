find_package(Python 3.10 COMPONENTS Interpreter Development.Module REQUIRED)
# Detect the installed nanobind package and import it into CMake
execute_process(
  COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
  OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE NB_DIR)
list(APPEND CMAKE_PREFIX_PATH "${NB_DIR}")
find_package(nanobind CONFIG REQUIRED)
nanobind_add_module(bound bind.cpp)
target_link_libraries(bound LINK_PUBLIC Engine)
add_compile_options(-fPIC)
