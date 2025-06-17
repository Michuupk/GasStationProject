cmake_minimum_required(VERSION 3.31.6)
project(gas_station)

set(CMAKE_CXX_STANDARD 17)

include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(gas_station main.cpp
        src/Car.cpp
        src/Cargo_Car.cpp
        src/Gas_Station.cpp
        src/Simulation.cpp
        src/Tanker.cpp
        src/Washing_Station.cpp)
