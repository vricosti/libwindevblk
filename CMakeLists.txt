cmake_minimum_required(VERSION 3.1)

#Configuration du projet
project(libwindevblk)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)

add_subdirectory(src)
add_subdirectory(test)


