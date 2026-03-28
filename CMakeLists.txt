cmake_minimum_required(VERSION 4.2)
project(Voxian)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE VOXIAN_SOURCES
    "Code/**.cpp"
)

add_executable(voxian ${VOXIAN_SOURCES})
target_include_directories(voxian PRIVATE "Code")

add_custom_target(copy_assets
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/Assets ${CMAKE_CURRENT_BINARY_DIR}/Assets
)
add_dependencies(voxian copy_assets)

add_subdirectory("Deps/glad")
find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)

target_link_libraries(voxian PRIVATE
    glfw
    glad
    spdlog::spdlog
)
