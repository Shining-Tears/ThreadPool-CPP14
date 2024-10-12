cmake_minimum_required(VERSION 3.18)
project(ThreadPoolCPP14 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

message("Current Path: ${CMAKE_CURRENT_SOURCE_DIR}")

file(GLOB SOURCE_CODE "${CMAKE_CURRENT_SOURCE_DIR}/src/*cpp")

add_executable(ThreadPoolCPP14 ${SOURCE_CODE})

target_include_directories(ThreadPoolCPP14 PRIVATE 
            ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_libraries(ThreadPoolCPP14 PRIVATE 
            ${EXT_LIBS})