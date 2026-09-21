cmake_minimum_required(VERSION 3.28)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 使用本地 SFML 源码
set(SFML_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sfml")
if(NOT EXISTS "${SFML_SOURCE_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "找不到 SFML 源码目录。请下载 SFML-3.1.0-sources.zip 并解压到 ${SFML_SOURCE_DIR}")
endif()
add_subdirectory(${SFML_SOURCE_DIR})

if(TARGET mbedtls)
    target_compile_options(mbedtls PRIVATE -w)
endif()

add_executable(main src/main.cpp)
target_compile_features(main PRIVATE cxx_std_17)
target_link_libraries(main PRIVATE SFML::Graphics)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT main)
