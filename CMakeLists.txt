cmake_minimum_required(VERSION 3.10)
project(heliox)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDRAD_REQUIRED ON)

include_directories(${PROJECT_SOURCE_DIR}/src)

file(GLOB SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)

add_executable(heliox ${SOURCES})
