cmake_minimum_required(VERSION 3.10)
project(PufflePaddle)

set(CMAKE_CXX_STANDARD 17)

# Diz ao CMake para gerar o compile_commands.json para o clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Encontra o OpenCV
find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)

# Cria o executável
add_executable(main src/main.cpp)
add_compile_options(-O2)

# Adiciona os diretórios de inclusão do OpenCV
include_directories(main PRIVATE ${OpenCV_INCLUDE_DIRS} ${X11_INCLUDE_DIR})

# Linka as bibliotecas do OpenCV ao executável
target_link_libraries(main PRIVATE
    ${OpenCV_LIBS}
    ${X11_LIBRARIES}
    ${X11_Xext_LIB}
)
