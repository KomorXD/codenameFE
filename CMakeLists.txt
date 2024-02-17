cmake_minimum_required(VERSION 3.8)
project("codename.FE" VERSION 0.0.1)

enable_testing()

add_subdirectory("src")
add_subdirectory("tests")
add_subdirectory("extern/glfw")
add_subdirectory("extern/glm")
add_subdirectory("extern/spdlog")