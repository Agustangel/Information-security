cmake_minimum_required(VERSION 3.10)
project(SecureCalculator)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Директория с заголовочными файлами
include_directories(include)

# Исходные файлы
set(SOURCES
    src/main.cpp
    src/auth_manager.cpp
    src/calculator_engine.cpp
    src/menu_manager.cpp
)

# Создание исполняемого файла
add_executable(SecureCalculator ${SOURCES})

# Настройки компилятора
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SecureCalculator PRIVATE -Wall -Wextra -Wpedantic -std=c++23)
endif()

# Настройки для Linux (необходимые библиотеки)
if(UNIX AND NOT APPLE)
    target_link_libraries(SecureCalculator m)
endif()