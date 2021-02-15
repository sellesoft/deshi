cmake_minimum_required(VERSION 3.7.0)

set(NAME deshi)
project(${NAME})

list(APPEND LIBS )

include_directories(src/)
include_directories(C:\src\boost_1_74_0)
include_directories(C:\src\glfw-3.3.2.bin.WIN64\include)
include_directories(C:\src\glm)
include_directories(C:\src\OpenAL1.1\include)
include_directories(C:\src\stb-master)

file(GLOB_RECURSE "src/*.cpp")

add_executable(Deshi ${SOURCES})