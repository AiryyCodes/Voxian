cmake_minimum_required(VERSION 4.2)
project(Stb)

add_library(stb STATIC "stb/src/stb_image.c")

target_include_directories(stb PUBLIC "stb/include/")