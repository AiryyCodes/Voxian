cmake_minimum_required(VERSION 4.2)
project(Voxian)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE VOXIAN_SOURCES
    "Code/**.cpp"
)

add_executable(voxian ${VOXIAN_SOURCES})
target_include_directories(voxian PRIVATE "Code")

add_subdirectory("Deps/glad")
find_package(glfw3 REQUIRED)

target_link_libraries(voxian PRIVATE
    glfw
    glad
)
