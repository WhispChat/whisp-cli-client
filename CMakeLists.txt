cmake_minimum_required(VERSION 3.12)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

project(
  WhispCLI
  VERSION 0.1
  DESCRIPTION "Whisp chat command line interface client"
  LANGUAGES CXX)

add_subdirectory(src)
