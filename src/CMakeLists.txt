# Cria a biblioteca interna com os arquivos de código-fonte
add_library(self_driven_car_c
    activation_relu.cpp
    activation_sigmoid.cpp
    activation_softmax.cpp
    activation_tanh.cpp
    layer.cpp
    loss.cpp
    mathematics.cpp
    game.cpp
    car.cpp
)

# Cria o executável principal (main.cpp)
add_executable(${PROJECT_NAME} ${CMAKE_SOURCE_DIR}/main.cpp)

# Vincula o executável com a biblioteca interna e com as bibliotecas do SFML utilizando os targets importados
target_link_libraries(${PROJECT_NAME} PRIVATE self_driven_car_c SFML::Graphics SFML::Window SFML::System)

# Define o diretório de include para a biblioteca interna
target_include_directories(self_driven_car_c PUBLIC ${CMAKE_SOURCE_DIR}/include)
