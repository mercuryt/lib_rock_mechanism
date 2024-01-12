add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)
set(CMAKE_CXX_FLAGS_DEBUG  "-g")
add_subdirectory(unit)
