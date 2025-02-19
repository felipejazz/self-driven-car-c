add_library(self_driven_car_c
    activation_relu.cpp
    activation_sigmoid.cpp
    activation_softmax.cpp
    activation_tanh.cpp
    layer.cpp
    loss.cpp
    mathematics.cpp
)

target_include_directories(self_driven_car_c PUBLIC ${CMAKE_SOURCE_DIR}/include)
