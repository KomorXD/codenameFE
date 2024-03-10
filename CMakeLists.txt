cmake_minimum_required(VERSION 3.8)
project("codename.FE" VERSION 0.0.1)

add_definitions(-D_SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING)

add_subdirectory("src")
add_subdirectory("extern/glfw")
add_subdirectory("extern/glm")
add_subdirectory("extern/spdlog")
add_subdirectory("tests")

if(MSVC AND NOT WIN32)
    set(GLAD_LIBRARIES dl)
endif()