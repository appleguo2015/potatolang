cmake_minimum_required(VERSION 3.16)
project(potatolang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(potatolang main.cpp)
target_include_directories(potatolang PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(potatolang PRIVATE ${SDL2_LIBRARIES})

add_executable(tomato tomato/main.cpp)
target_include_directories(tomato PRIVATE ${SDL2_INCLUDE_DIRS})
target_link_libraries(tomato PRIVATE ${SDL2_LIBRARIES})
