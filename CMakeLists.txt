cmake_minimum_required(VERSION 3.10)
project(PufflePaddle)

set(CMAKE_CXX_STANDARD 17)

# Diz ao CMake para gerar o compile_commands.json para o clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Encontra o OpenCV
find_package(OpenCV REQUIRED)

# Adiciona os diretórios de inclusão do OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Cria o executável
add_executable(main prototype/main.cpp)

# Linka as bibliotecas do OpenCV ao executável
target_link_libraries(main ${OpenCV_LIBS})
