
cmake_minimum_required(VERSION 3.10)
project(SelfDrivenCar)


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/third_party/eigen)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)
