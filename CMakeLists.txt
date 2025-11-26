cmake_minimum_required(VERSION 3.28)
project(Voxian CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

file(GLOB_RECURSE GAME_SOURCES
    "Code/**.cpp"
)

add_custom_target(link_target ALL
    COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/Assets ${CMAKE_BINARY_DIR}/Assets)

add_executable(${PROJECT_NAME} ${GAME_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "Code")

if (CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "")
    # Use -O1 (or /O1 for MSVC) for basic speed optimizations
    target_compile_options(${PROJECT_NAME} PRIVATE 
        $<$<COMPILE_LANGUAGE:CXX>:$<$<CONFIG:Debug>:-O1>>
    )
    
    # Disable expensive C++ Standard Library Debug Checks (Huge Performance Boost)
    # This disables iterator safety and bounds checking in containers like std::vector.
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=0>
    )

    # Define a preprocessor flag to enable/disable expensive custom runtime checks
    # You can now wrap expensive custom assertions or memory fills with #ifndef FAST_DEBUG
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Debug>:FAST_DEBUG>
    )
endif()

add_subdirectory("Deps/glad")
add_subdirectory("Deps/stb")
add_subdirectory("Deps/FastNoiseLite")

find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad spdlog fmt glm stb FastNoiseLite nlohmann_json)
