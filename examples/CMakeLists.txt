
add_executable(example1 src/example1.cpp)

target_link_libraries(example1 PUBLIC bs11n mongo::bsoncxx_shared)