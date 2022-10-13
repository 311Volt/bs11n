cmake_minimum_required(VERSION 3.0.0)
project(bs11n VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)

find_package(mongocxx REQUIRED)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(bs11n INTERFACE include/bs11n.hpp)
target_include_directories(bs11n INTERFACE include)

add_subdirectory(examples)

target_link_libraries(bs11n INTERFACE mongo::bsoncxx_static)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
