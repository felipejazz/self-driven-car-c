cmake_minimum_required(VERSION 3.10)
project(SelfDrivenCar)

set(CMAKE_CXX_STANDARD 17)

# Procura a instalação do SFML (versão 3.0 ou superior) com os módulos necessários.
# Note que, para o SFML 3, os nomes dos componentes são case-sensitive.
find_package(SFML 3 REQUIRED COMPONENTS Graphics Window System)

# Diretórios de includes
include_directories(${CMAKE_SOURCE_DIR}/include)

# Adicionar o diretório do Eigen instalado no sistema
include_directories(/usr/include/eigen3)

# Adiciona o diretório com o código-fonte
add_subdirectory(src)

# Configuração para testes
enable_testing()
add_subdirectory(tests)