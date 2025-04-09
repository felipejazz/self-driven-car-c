
add_library(self_driven_car_c
    layer.cpp
    mathematics.cpp
    game.cpp
    car.cpp
    road.cpp
    sensor.cpp
    util.cpp
    controls.cpp
    neural_network.cpp
)

add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE self_driven_car_c SFML::Graphics SFML::Window SFML::System)

target_include_directories(self_driven_car_c PUBLIC ${CMAKE_SOURCE_DIR}/include)
