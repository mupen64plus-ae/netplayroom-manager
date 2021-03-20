#
# Mupen64PlusAE, an N64 emulator for the Android platform
#
# Copyright (C) 2021 Francisco Zurita
#
# This file is part of Mupen64PlusAE.
#
# Mupen64PlusAE is free software: you can redistribute it and/or modify it under the terms of the
# GNU General Public License as published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# Mupen64PlusAE is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
# without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with Mupen64PlusAE. If
# not, see <http://www.gnu.org/licenses/>.
#
# Authors: fzurita
#
 
cmake_minimum_required(VERSION 3.10)
project(netplayroom-manager)

add_definitions("-std=c++17")
add_compile_definitions(SPDLOG_ACTIVE_LEVEL=0)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

set(NP_ROOM_MANAGER_SOURCES
    src/main.cpp
    src/TcpSocketHandler.cpp
    src/ClientHandler.cpp
    src/RoomManager.cpp
)

add_executable(np-room-manager ${NP_ROOM_MANAGER_SOURCES})
target_link_libraries(np-room-manager ${CONAN_LIBS})

