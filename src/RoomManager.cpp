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

#include <limits>
#include "RoomManager.hpp"

RoomManager::RoomManager() :
    mMt(mRandomDevice()),
    mDistribution(std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max())
{
}

uint32_t RoomManager::createRoom(std::string ipAddress, int port)
{
    auto roomValue = std::make_pair(ipAddress, port);
    
    uint32_t roomNumber = mDistribution(mMt);
    
    // Find an unused roomNumber
    while (mRoomNumbers.count(roomNumber) != 0) {
        roomNumber = mDistribution(mMt);
    }
    
    mRoomNumbers[roomNumber] = roomValue;
    
    return roomNumber;
}

std::pair<std::string, int> RoomManager::getRoom(uint32_t roomNumber)
{
    std::pair<std::string, int> roomData = std::make_pair("", -1);

    if (mRoomNumbers.count(roomNumber) != 0) {
        roomData = mRoomNumbers[roomNumber];
    }
        
    return roomData;
}

void RoomManager::removeRoom(uint32_t roomNumber)
{
    mRoomNumbers.erase(roomNumber);
}