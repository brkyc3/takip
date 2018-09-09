cmake_minimum_required(VERSION 3.10)
project(cinbekleme)
find_package(OpenCV REQUIRED)
set(CMAKE_CXX_STANDARD 11)

add_executable(cinbekleme main.cpp)

target_link_libraries(cinbekleme ${OpenCV_LIBS})
