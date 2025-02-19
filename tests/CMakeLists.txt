cmake_minimum_required(VERSION 3.10)

add_executable(test_mathematics test_mathematics.cpp)

target_include_directories(test_mathematics PRIVATE ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(test_mathematics PRIVATE self_driven_car_c)
