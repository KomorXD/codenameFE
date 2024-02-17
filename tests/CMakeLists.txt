set(gtest_force_shared_crt on)
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.11.0
)
FetchContent_MakeAvailable(googletest)

file(GLOB PROJECT_SOURCES_TEST "*.cpp")

add_executable(${PROJECT_NAME}-Tests)
target_sources(${PROJECT_NAME}-Tests
    PRIVATE
        ${PROJECT_SOURCES_TEST}
)

target_include_directories(${PROJECT_NAME}-Tests
    PRIVATE
        "${CMAKE_SOURCE_DIR}/src/"
)
target_link_libraries(${PROJECT_NAME}-Tests gtest gtest_main)
target_link_libraries(${PROJECT_NAME}-Tests ${PROJECT_NAME}-Lib)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}-Tests)