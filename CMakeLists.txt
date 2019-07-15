cmake_minimum_required(VERSION 3.8)
project(homegear_knx)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        src/PhysicalInterfaces/MainInterface.cpp
        src/PhysicalInterfaces/MainInterface.h
        src/DptConverter.cpp
        src/DptConverter.h
        src/Factory.cpp
        src/Factory.h
        src/KnxIpForwarder.cpp
        src/KnxIpForwarder.h
        src/GD.cpp
        src/GD.h
        src/Interfaces.cpp
        src/Interfaces.h
        src/KnxCentral.cpp
        src/KnxCentral.h
        src/Knx.cpp
        src/Knx.h
        src/Cemi.cpp
        src/Cemi.h
        src/KnxIpPacket.cpp
        src/KnxIpPacket.h
        src/KnxPeer.cpp
        src/KnxPeer.h
        src/Search.cpp
        src/Search.h
        config.h src/KnxIpPacket.cpp src/KnxIpPacket.h)

add_custom_target(homegear COMMAND ../../makeAll.sh SOURCES ${SOURCE_FILES})

add_library(homegear_knx ${SOURCE_FILES})