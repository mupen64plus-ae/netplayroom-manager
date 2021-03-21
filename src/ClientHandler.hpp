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

#include <array>
#include <unordered_map>
#include <mutex>

#include "RoomManager.hpp"

class ClientHandler
{
public:
    
    /**
     * Constructor
     * @param roomManager Room manager
     * @param socketHandle Socket handle associated with this client
     */
    ClientHandler(RoomManager& roomManager, int socketHandle);
    
    /**
     * Copy constructor
     */
    ClientHandler ( const ClientHandler & _clientHandler);
    
    /**
     * Move constructor
     */
    ClientHandler ( const ClientHandler && _clientHandler);
    
    /**
     * Destructor
     */
    ~ClientHandler();
    
    /**
     * Process any data available in the stream
     * @return true if the connection needs to be closed
     */
    bool processStream();
    
    /**
     * If a netplay server has registered with us, this will send them the room number
     * if a connection is present
     */
    void sendNetplayRoomIfConnected();
	
private:
    
    /**
     * Process a pending message
     * @param messageId Message id to process
     * @return true if response was successfully sent
     */
    bool processPendingMessage(int messageId);
    
    /**
     * Handle a init session message
     * @return true if response was successfully sent
     */
    bool handleInitSession();
    
    /**
     * Handle a register netplay server message
     * @return true if response was successfully sent
     */
    bool handleRegisterNpServer();
    
    /**
     * Handle a netplay server game started message
     * @return true if response was successfully sent
     */
    bool handleNpServerGameStarted();
    
    /**
     * Handle a netplay client request registration message
     * @return true if response was successfully sent
     */
    bool handleNpClientRequestRegistration();
    
    // Message Ids
    enum MessageIds {
        INIT_SESSION = 0,
        REGISTER_NP_SERVER = 1,
        NP_SERVER_GAME_STARTED = 2,
        NP_CLIENT_REQUEST_REGISTRATION = 3,
        INIT_SESSION_RESPONSE = 100,
        REGISTER_NP_SERVER_RESPONSE = 101,
        NP_CLIENT_REQUEST_REGISTRATION_RESPONSE = 103
    };
    
    // Size of message ID in all messages
    static const int MESSAGE_ID_SIZE_BYTES = 4;
    
    // Netplay version
    static const uint32_t NETPLAY_VERSION = 2;
    
    // Map of message ids to size
    static std::unordered_map<int,int> mMessageIdToSize;
    
    // Socket handle associated with this client
    int mSocketHandle;
    
    // Socket handle used to send the room number
    int mSocketHandleSendRoomNumber;
    
    // Buffer used for receiving data
    std::array<char,100> mReceiveBuffer;
    
    // Buffer used for sending data
    std::array<char,100> mSendBuffer;
    
    // Buffer used for sending registration response data
    std::array<char,8> mRegistrationResponse;
    
    // Current offset into the buffer for receiving data
    int mCurrentBufferOffset;
    
    // The message size for the current message being processed
    int mCurrentMessageSize;
    
    // Room manager
    RoomManager& mRoomManager;
    
    // Room number
    uint32_t mRoomNumber;
    
    // True if room number has been sent
    bool mRoomNumberSent;
    
    // Current byte offset of registration response message
    int mRoomNumberSentBytes;
    
    // Mutex for protecting client room and connection
    std::mutex mClientRoomMutex;
    
    // True if the session has been initialized
    bool mHasBeenInit;
};