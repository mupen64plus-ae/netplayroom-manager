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

#include "ClientHandler.hpp"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <algorithm>
#include "spdlog/spdlog.h"

std::unordered_map<int,int> ClientHandler::mMessageIdToSize;

ClientHandler::ClientHandler(RoomManager& roomManager, int socketHandle) :
    mSocketHandle(socketHandle),
    mSocketHandleSendRoomNumber(-1),
    mCurrentBufferOffset(0),
    mCurrentMessageSize(-1),
    mRoomManager(roomManager),
    mRoomNumber(0),
    mRoomNumberSent(false),
    mRoomNumberSentBytes(0),
    mHasBeenInit(false)
{
    if (mMessageIdToSize.empty())
    {
        mMessageIdToSize[INIT_SESSION] = 8;
        mMessageIdToSize[REGISTER_NP_SERVER] = 8;
        mMessageIdToSize[NP_SERVER_GAME_STARTED] = 4;
        mMessageIdToSize[NP_CLIENT_REQUEST_REGISTRATION] = 8;
    }
}

ClientHandler::ClientHandler( const ClientHandler & _clientHandler) :
    mSocketHandle(_clientHandler.mSocketHandle),
    mSocketHandleSendRoomNumber(_clientHandler.mSocketHandleSendRoomNumber),
    mReceiveBuffer(_clientHandler.mReceiveBuffer),
    mSendBuffer(_clientHandler.mSendBuffer),
    mRegistrationResponse(_clientHandler.mRegistrationResponse),
    mCurrentBufferOffset(_clientHandler.mCurrentBufferOffset),
    mCurrentMessageSize(_clientHandler.mCurrentMessageSize),
    mRoomManager(_clientHandler.mRoomManager),
    mRoomNumber(_clientHandler.mRoomNumber),
    mRoomNumberSent(_clientHandler.mRoomNumberSent),
    mRoomNumberSentBytes(_clientHandler.mRoomNumberSentBytes)
{

}

ClientHandler::ClientHandler( const ClientHandler && _clientHandler) :
    mSocketHandle(std::move(_clientHandler.mSocketHandle)),
    mSocketHandleSendRoomNumber(std::move(_clientHandler.mSocketHandleSendRoomNumber)),
    mReceiveBuffer(std::move(_clientHandler.mReceiveBuffer)),
    mSendBuffer(std::move(_clientHandler.mSendBuffer)),
    mRegistrationResponse(std::move(_clientHandler.mRegistrationResponse)),
    mCurrentBufferOffset(std::move(_clientHandler.mCurrentBufferOffset)),
    mCurrentMessageSize(std::move(_clientHandler.mCurrentMessageSize)),
    mRoomManager(_clientHandler.mRoomManager),
    mRoomNumber(std::move(_clientHandler.mRoomNumber)),
    mRoomNumberSent(std::move(_clientHandler.mRoomNumberSent)),
    mRoomNumberSentBytes(std::move(_clientHandler.mRoomNumberSentBytes))
{
}

ClientHandler::~ClientHandler()
{
    if (mSocketHandleSendRoomNumber != -1) {
        close(mSocketHandleSendRoomNumber);
    }
    
    mRoomManager.removeRoom(mRoomNumber);
}

bool ClientHandler::processStream()
{
    bool closeConn = false;
    
    // Receive data on this connection until the recv fails with EWOULDBLOCK. If any other
    // failure occurs, we will close the connection.
    while (true) {
        
        int receivedBytes = recv(mSocketHandle, mReceiveBuffer.data() + mCurrentBufferOffset, mReceiveBuffer.size() - mCurrentBufferOffset, 0);

        if (receivedBytes < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                SPDLOG_ERROR("recv() failed on socket {}", mSocketHandle);
                closeConn = true;
            }
            break;
        } 
        // Check to see if the connection has been closed by the client
        if (receivedBytes == 0)
        {
            SPDLOG_ERROR("Connection closed for {}", mSocketHandle);
            closeConn = true;
            mSocketHandleSendRoomNumber = -1;
            break;
        }
        
        // Data was received
        mCurrentBufferOffset += receivedBytes;
        
        uint32_t messageId = -1;
        if (mCurrentBufferOffset >= MESSAGE_ID_SIZE_BYTES) {
            messageId = ntohl(*reinterpret_cast<uint32_t*>(mReceiveBuffer.data()));
            
            if (mMessageIdToSize.count(messageId) != 0) {
                mCurrentMessageSize = mMessageIdToSize[messageId];
                if (mCurrentMessageSize > mReceiveBuffer.size()) {
                    SPDLOG_ERROR("Unexpected message size for message id {}", messageId);
                    closeConn = true;
                    break;
                }
                
            } else {
                SPDLOG_ERROR("Received invalid message id {}", messageId);
                closeConn = true;
                break;
            }
        }
        
        // The full message has been received, process it
        if (mCurrentBufferOffset >= mCurrentMessageSize) {
            closeConn = !processPendingMessage(messageId);
            
            mCurrentBufferOffset -= mCurrentMessageSize;
        }
    }
    
    return closeConn;
}


bool ClientHandler::processPendingMessage(int messageId)
{
    switch(messageId) {
        case INIT_SESSION:
            return handleInitSession();
        case REGISTER_NP_SERVER:
            return handleRegisterNpServer();
        case NP_SERVER_GAME_STARTED:
            return handleNpServerGameStarted();
        case NP_CLIENT_REQUEST_REGISTRATION:
            return handleNpClientRequestRegistration();
        default:
            // Do nothing
            return true;
    }
}

bool ClientHandler::handleInitSession()
{
    bool sendSuccess = true;

    // Parse the message
    char* receiveBufferOffset = mReceiveBuffer.data();
    receiveBufferOffset += MESSAGE_ID_SIZE_BYTES; // Skip the message id
    uint32_t netplayVersion = ntohl(*reinterpret_cast<uint32_t*>(receiveBufferOffset));
    
    // Send the response
    int sendBufferOffset = 0;
    uint32_t messageId = htonl(INIT_SESSION_RESPONSE);
    std::copy_n(reinterpret_cast<char*>(&messageId), sizeof(uint32_t), mSendBuffer.data() + sendBufferOffset);
    sendBufferOffset += sizeof(uint32_t);

    mHasBeenInit = netplayVersion == NETPLAY_VERSION;
    bool validVersion = htonl(mHasBeenInit);
    std::copy_n(reinterpret_cast<char*>(&validVersion), sizeof(uint32_t), mSendBuffer.data() + sendBufferOffset);
    sendBufferOffset += sizeof(uint32_t);
    
    int sentBytes = send(mSocketHandle, mSendBuffer.data(), sendBufferOffset, 0);

    if (sentBytes < 0)
    {
        sendSuccess = false;
        SPDLOG_ERROR("Unable to send init session response message");
    }
    
    if (!mHasBeenInit) {
        sendSuccess = false;
    }
    
    return sendSuccess;
}

bool ClientHandler::handleRegisterNpServer()
{
    if (!mHasBeenInit) {
        return false;
    }
    
    std::unique_lock<std::mutex> lock(mClientRoomMutex);
    
    bool sendSuccess = true;

    // Parse the message
    char* receiveBufferOffset = mReceiveBuffer.data();
    receiveBufferOffset += MESSAGE_ID_SIZE_BYTES; // Skip the message id
    uint32_t netplayServerPort = ntohl(*reinterpret_cast<uint32_t*>(receiveBufferOffset));
    receiveBufferOffset += sizeof(uint32_t);
    
    sockaddr_storage addrStorage;
    socklen_t len = sizeof(sockaddr_storage);
    getpeername(mSocketHandle, reinterpret_cast<sockaddr*>(&addrStorage), &len);
    
    char ipAddress[INET6_ADDRSTRLEN];
    sockaddr_in6& address = *reinterpret_cast<sockaddr_in6*>(&addrStorage);
    inet_ntop(AF_INET6, &address.sin6_addr, ipAddress, sizeof(ipAddress));
    
    // Create the room
    mRoomNumber = mRoomManager.createRoom(std::string(ipAddress), netplayServerPort);
    SPDLOG_INFO("Created room {} on socket {}: {}:{}", mRoomNumber, mSocketHandle, ipAddress, netplayServerPort);

    mSocketHandleSendRoomNumber = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
      
    // Set socket to be nonblocking.
    int on = 1;
    if (ioctl(mSocketHandleSendRoomNumber, FIONBIO, reinterpret_cast<char*>(&on)) < 0)
    {
        SPDLOG_ERROR("ioctl() failed");
        close(mSocketHandleSendRoomNumber);
        return false;
    }
    
    sockaddr_in6 server_addr;
    server_addr.sin6_family = AF_INET6;
    inet_pton(AF_INET6, ipAddress, &server_addr.sin6_addr);
    server_addr.sin6_port = htons(netplayServerPort); 
    
    if (connect(mSocketHandleSendRoomNumber, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {

       if (errno != EWOULDBLOCK && errno != EINPROGRESS)
       {
           SPDLOG_ERROR("connect() failed on socket {} address: {}:{}, error={}", mSocketHandle, ipAddress, netplayServerPort, strerror(errno));
           return false;
       }
    }

    int sendBufferOffset = 0;
    uint32_t messageId = htonl(REGISTER_NP_SERVER_RESPONSE);
    std::copy_n(reinterpret_cast<char*>(&messageId), sizeof(uint32_t), mRegistrationResponse.data() + sendBufferOffset);
    sendBufferOffset += sizeof(uint32_t);

    uint32_t roomNumber = htonl(mRoomNumber);
    std::copy_n(reinterpret_cast<char*>(&roomNumber), sizeof(uint32_t), mRegistrationResponse.data() + sendBufferOffset);

    return true;
}

bool ClientHandler::handleNpServerGameStarted()
{
    // No response, just remove the room and close the connection
    mRoomManager.removeRoom(mRoomNumber);
    
    return false;
}

bool ClientHandler::handleNpClientRequestRegistration()
{
    if (!mHasBeenInit) {
        return false;
    }

    bool sendSuccess = true;

    // Parse the message
    char* receiveBufferOffset = mReceiveBuffer.data();
    receiveBufferOffset += MESSAGE_ID_SIZE_BYTES; // Skip the message id
    uint32_t roomId = ntohl(*reinterpret_cast<uint32_t*>(receiveBufferOffset));
    
    // Send the response
    int sendBufferOffset = 0;
    uint32_t messageId = htonl(NP_CLIENT_REQUEST_REGISTRATION_RESPONSE);
    std::copy_n(reinterpret_cast<char*>(&messageId), sizeof(uint32_t), mSendBuffer.data() + sendBufferOffset);
    sendBufferOffset += sizeof(uint32_t);
    
    // Get IP and port
    auto roomData = mRoomManager.getRoom(roomId);

    char ipAddress[INET6_ADDRSTRLEN];
    std::fill(ipAddress, ipAddress + INET6_ADDRSTRLEN, 0);
    roomData.first.copy(ipAddress, INET6_ADDRSTRLEN);
    std::copy_n(ipAddress, sizeof(ipAddress), mSendBuffer.data() + sendBufferOffset);
    sendBufferOffset += sizeof(ipAddress);
    
    int32_t port = htonl(roomData.second);
    std::copy_n(reinterpret_cast<char*>(&port), sizeof(port), mSendBuffer.data() + sendBufferOffset);
    sendBufferOffset += sizeof(port);

    SPDLOG_ERROR("Request for room data on socket {}, room={}, ip={}, port={}", mSocketHandle, roomId, ipAddress, roomData.second);

    
    int sentBytes = send(mSocketHandle, mSendBuffer.data(), sendBufferOffset, 0);

    if (sentBytes < 0)
    {
        sendSuccess = false;
        SPDLOG_ERROR("Unable to send registration data request response");
    }
    
    return sendSuccess;
}

void ClientHandler::sendNetplayRoomIfConnected()
{
    std::unique_lock<std::mutex> lock(mClientRoomMutex);
    
    if (mSocketHandleSendRoomNumber != -1 && !mRoomNumberSent) {
        
        sockaddr_storage addrStorage;
        socklen_t len = sizeof(sockaddr_storage);
        int returnCode = getpeername(mSocketHandleSendRoomNumber, reinterpret_cast<sockaddr*>(&addrStorage), &len);

        // Send the response if we are connected
        if (returnCode == 0) {
            int sentBytes = send(mSocketHandleSendRoomNumber, mRegistrationResponse.data() + mRoomNumberSentBytes,
                mRegistrationResponse.size() - mRoomNumberSentBytes, 0);
        
            if (sentBytes < 0)
            {
                SPDLOG_ERROR("Unable to send registration response, errno={}, str={}", errno, strerror(errno));
                close(mSocketHandleSendRoomNumber);
                mSocketHandleSendRoomNumber = -1;
            }
            else
            {
                mRoomNumberSentBytes += sentBytes;
                
                if (mRoomNumberSentBytes == mRegistrationResponse.size()) {
                    mRoomNumberSent = true;
                }
            }
        }    
    }
}


