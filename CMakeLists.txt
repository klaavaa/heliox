cmake_minimum_required(VERSION 4.0)


project(heliox LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(heliox ${SOURCES})

target_include_directories(heliox PRIVATE src)

if (MSVC)
    target_compile_options(heliox PRIVATE /W4)
else()
    target_compile_options(heliox PRIVATE -Wall -Wextra -Wpedantic)
endif()




