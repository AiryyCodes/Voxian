cmake_minimum_required(VERSION 4.2)
project(Voxian)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE VOXIAN_SOURCES
    "Code/**.cpp"
)

add_executable(${PROJECT_NAME} ${VOXIAN_SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "Code")

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets
)
add_dependencies(${PROJECT_NAME} copy_assets)

include(cmake/glaze.cmake)

add_subdirectory("Deps/glad")
add_subdirectory("Deps/stb")
add_subdirectory("Deps/FastNoiseLite")
find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    stb
    FastNoiseLite
    spdlog::spdlog
    glm::glm
    glaze::glaze
)
