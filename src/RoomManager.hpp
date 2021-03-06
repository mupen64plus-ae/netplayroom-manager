/*
 * Mupen64PlusAE, an N64 emulator for the Android platform
 *
 * Copyright (C) 2021 Francisco Zurita
 *
 * This file is part of Mupen64PlusAE.
 *
 * Mupen64PlusAE is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Mupen64PlusAE is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with Mupen64PlusAE. If
 * not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: fzurita
 */

#pragma once

#include <unordered_map>
#include <utility>
#include <random>

class RoomManager
{
public:
    
    /**
     * Constructor
     */
    RoomManager();
    
    /**
     * Creates a room using the given IP and port and returns the room number
     * @param ipAddress IP Address of room
     * @param port Port number of room
     * @return Randomly generated room number
     */
    uint32_t createRoom(std::string ipAddress, int port);
    
    /**
     * Gets room data given a room number
     * @param roomNumber Room number
     * @return Pair of address and port, if the room is not found, port will be -1
     */
    std::pair<std::string, int> getRoom(uint32_t roomNumber);
    
    /**
     * Removes a room using the room number
     * @param roomNumber Room number to remove
     */
    void removeRoom(uint32_t roomNumber);
	
private:
    // Random device
    std::random_device mRandomDevice;
    
    // Random "mt"
    std::mt19937 mMt;
    
    // Random distribution
    std::uniform_int_distribution<uint32_t> mDistribution;
    
    // Map of room number to Room ip and port
    std::unordered_map<uint32_t, std::pair<std::string, int>> mRoomNumbers;
};