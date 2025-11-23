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

add_subdirectory("Deps/glad")
add_subdirectory("Deps/stb")
add_subdirectory("Deps/FastNoiseLite")

find_package(glfw3 REQUIRED)
find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(${PROJECT_NAME} PRIVATE glfw glad spdlog fmt glm stb FastNoiseLite)
