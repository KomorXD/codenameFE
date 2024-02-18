set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(gtest_force_shared_crt on)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(googletest)

file(GLOB PROJECT_SOURCES_TEST "*.cpp")

enable_testing()

add_executable(${PROJECT_NAME}-Tests)
target_sources(${PROJECT_NAME}-Tests
    PRIVATE
        ${PROJECT_SOURCES_TEST}
)

target_include_directories(${PROJECT_NAME}-Tests
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/"
        "${CMAKE_SOURCE_DIR}/extern/glfw/include/"
        "${CMAKE_SOURCE_DIR}/extern/glad/include/"
        "${CMAKE_SOURCE_DIR}/extern/glm/"
        "${CMAKE_SOURCE_DIR}/extern/spdlog/include/"
)
target_link_libraries(${PROJECT_NAME}-Tests gtest gtest_main)
target_link_libraries(${PROJECT_NAME}-Tests ${PROJECT_NAME}-Lib)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}-Tests)