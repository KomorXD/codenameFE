set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(GLFW_BUILD_DOCS OFF)
option(GLFW_BUILD_EXAMPLES OFF)
option(GLFW_BUILD_TESTS OFF)

add_library(${PROJECT_NAME}-Lib STATIC)
add_executable(${PROJECT_NAME} "Main.cpp")

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS "*.hpp")
file(GLOB_RECURSE VENDORS_SOURCES CONFIGURE_DEPENDS 
    "${CMAKE_SOURCE_DIR}/extern/glad/src/glad.c"
)
file(GLOB_RECURSE PROJECT_CONFIGS CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/CMakeLists.txt"
    "${CMAKE_SOURCE_DIR}/.gitignore"
    "${CMAKE_SOURCE_DIR}/.gitmodules"
)

list(REMOVE_ITEM PROJECT_SOURCES "Main.cpp")

if(SSS_DEBUG)
	add_definitions(-DCONF_DEBUG=1)
elseif(SSS_RELEASE)
	add_definitions(-DCONF_RELEASE=1)
elseif(SSS_PROD)
	add_definitions(-DCONF_PROD=1)
endif()

if(TARGET_WINDOWS)
	add_definitions(-DTARGET_WINDOWS=1)
elseif(TARGET_LINUX)
	add_definitions(-DTARGET_LINUX=1)
endif()

target_sources(${PROJECT_NAME}-Lib
    PRIVATE
        ${PROJECT_SOURCES}
        ${PROJECT_HEADERS}
        ${VENDORS_SOURCES}
        ${PROJECT_CONFIGS}
)
target_include_directories(${PROJECT_NAME}-Lib
    PRIVATE
        "${CMAKE_SOURCE_DIR}/extern/glfw/include/"
        "${CMAKE_SOURCE_DIR}/extern/glad/include/"
        "${CMAKE_SOURCE_DIR}/extern/glm/"
        "${CMAKE_SOURCE_DIR}/extern/spdlog/include/"
)
target_link_libraries(${PROJECT_NAME}-Lib
    PRIVATE
        glfw
        ${GLFW_LIBRARIES}
        ${GLAD_LIBRARIES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_SOURCE_DIR}/extern/glfw/include/"
        "${CMAKE_SOURCE_DIR}/extern/glad/include/"
        "${CMAKE_SOURCE_DIR}/extern/glm/"
        "${CMAKE_SOURCE_DIR}/extern/spdlog/include/"
)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}-Lib)
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/out/bin/${BUILD_ARCHITECTURE}-${CMAKE_BUILD_TYPE}-${CMAKE_SYSTEM_NAME}/"
)