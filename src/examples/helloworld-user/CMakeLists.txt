# Minimal CMake file to use the HelloWorld installed package
cmake_minimum_required(VERSION 3.1)
project(Example VERSION 0.0.1 LANGUAGES CXX)
find_package(HelloWorld)
add_executable(main main.cpp)
target_link_libraries(main helloworld::HelloClasses)
