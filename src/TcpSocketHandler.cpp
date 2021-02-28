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

#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "spdlog/spdlog.h"

#include "TcpSocketHandler.hpp"

TcpSocketHandler::TcpSocketHandler(int portNumber) :
    mFds{}
{
    mPortNumber = portNumber;
    mEndServer = false;
    mNumberFileDescriptors = 1;
}

void TcpSocketHandler::startServer()
{
    int listenSd = -1;
    bool compressArray = false;

    sockaddr_in addr = {};
    
    // Create an AF_INET stream socket to receive incoming connections on
    listenSd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSd < 0)
    {
        SPDLOG_ERROR("socket() failed");
        return;
    }
  
    // Allow socket descriptor to be reuseable
    int on = 1;
    if (setsockopt(listenSd, SOL_SOCKET,  SO_REUSEADDR, reinterpret_cast<char*>(&on), sizeof(on)) < 0)
    {
        SPDLOG_ERROR("setsockopt() failed");
        close(listenSd);
        return;
    }
  
    // Set socket to be nonblocking. All of the sockets for the incoming connections will also be nonblocking since
    // they will inherit that state from the listening socket.
    if (ioctl(listenSd, FIONBIO, reinterpret_cast<char*>(&on)) < 0)
    {
        SPDLOG_ERROR("ioctl() failed");
        close(listenSd);
        return;
    }
  
    // Bind the socket
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    addr.sin_port = htons(mPortNumber);
    
    if (bind(listenSd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
    {
        SPDLOG_ERROR("bind() failed on port {}", mPortNumber);
        close(listenSd);
        return;
    }
  
    // Set the listen back log
    if (listen(listenSd, 32) < 0)
    {
      SPDLOG_ERROR("listen() failed");
      close(listenSd);
      return;
    }
  
    // Set up the initial listening socket
    mFds[0].fd = listenSd;
    mFds[0].events = POLLIN;
   
    // Loop waiting for incoming connects or for incoming data on any of the connected sockets.
    while (!mEndServer)
    {
        // Poll with a timeout of 1 second
        int pollReturn = poll(mFds.data(), mNumberFileDescriptors, 1);
    
        // Check to see if the poll call failed.
        if (pollReturn < 0)
        {
            SPDLOG_ERROR("poll() failed" );
            break;
        }
    
        // Check to see if timeout expired
        if (pollReturn == 0)
        {
            continue;
        }
    
        // One or more descriptors are readable.  Need to determine which ones they are.
        int currentSize = mNumberFileDescriptors;
        for (int fileDescriptors = 0; fileDescriptors < currentSize; fileDescriptors++)
        {
            // Loop through to find the descriptors that returned POLLIN and determine whether it's the listening
            // or the active connection.
            if (mFds[fileDescriptors].revents == 0)
            {
                continue;
            }

            // If revents is not POLLIN, it's an unexpected result, log and end the server.
            if(mFds[fileDescriptors].revents != POLLIN)
            {
                SPDLOG_ERROR("Error! revents = {}",  mFds[fileDescriptors].revents);
                mEndServer = true;
                break;
            }
  
            if (mFds[fileDescriptors].fd == listenSd)
            {
                if (!acceptNewConnections(listenSd))
                {
                    mEndServer = true;
                    break;
                }
            }
      
            // This is not the listening socket, therefore an existing connection must be readable 
            else
            {
                compressArray = compressArray || processData(mFds[fileDescriptors].fd);
            }
        }
    
        // If the compressArray flag was turned on, we need to squeeze together the array and decrement the number
        // of file descriptors. We do not need to move back the events and revents fields because the events will always
        // be POLLIN in this case, and revents is output.
        if (compressArray)
        {
            compressArray = false;
            for (int fileDescriptorIndex = 0; fileDescriptorIndex < mNumberFileDescriptors; fileDescriptorIndex++)
            {
                if (mFds[fileDescriptorIndex].fd == -1)
                {
                    for(int compressedIndex = fileDescriptorIndex; compressedIndex < mNumberFileDescriptors; compressedIndex++)
                    {
                        mFds[compressedIndex].fd = mFds[compressedIndex+1].fd;
                    }
                    fileDescriptorIndex--;
                    mNumberFileDescriptors--;
                }
            }
        }
    };

    // Clean up all of the sockets that are open
    for (int fileDescriptorIndex = 0; fileDescriptorIndex < mNumberFileDescriptors; fileDescriptorIndex++)
    {
        if(mFds[fileDescriptorIndex].fd >= 0)
        {
            close(mFds[fileDescriptorIndex].fd);
        }
    }
}

bool TcpSocketHandler::acceptNewConnections(int socketFd)
{
    bool success = true;
    
    // Listening descriptor is readable.
    SPDLOG_DEBUG("Listening socket is readable");    // Accept all incoming connections that are queued up on the listening socket before we
    // loop back and call poll again.
    int newSocket = accept(socketFd, nullptr, nullptr);
    
    while (newSocket != -1) {
        
        // Add the new incoming connection to the pollfd structure
        SPDLOG_INFO("New connection with id {}!", newSocket);
        mFds[mNumberFileDescriptors].fd = newSocket;
        mFds[mNumberFileDescriptors].events = POLLIN;
        mNumberFileDescriptors++;
        
        newSocket = accept(socketFd, nullptr, nullptr);
    }
    
    // If accept fails with EWOULDBLOCK, then we have accepted all of them. Any other failure
    // on accept will cause us to end the server.
    if (errno != EWOULDBLOCK)
    {
        SPDLOG_ERROR("accept() failed");
        success = false;
    }
    
    return success;
}

bool TcpSocketHandler::processData(int& socketFd)
{
    SPDLOG_DEBUG("Descriptor {} is readable",  socketFd);
    bool closeConn = false;
    char buffer[100];
    
    // Receive all incoming data on this socket before we loop back and call poll again.
    
    while(true)
    {
        // Receive data on this connection until the recv fails with EWOULDBLOCK. If any other
        // failure occurs, we will close the connection.
        int receiveBytes = recv(socketFd, buffer, sizeof(buffer), 0);
        
        if (receiveBytes < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                SPDLOG_ERROR("recv() failed");
                closeConn = true;
            }
            break;
        }
    
        // Check to see if the connection has been closed by the client
        if (receiveBytes == 0)
        {
            SPDLOG_ERROR("Connection closed for {}", socketFd);
            closeConn = true;
            break;
        }
    
        // Data was received
        int len = receiveBytes;
        SPDLOG_DEBUG("Received bytes {}", len);
    
        // Echo the data back to the client
        int sentBytes = send(socketFd, buffer, len, 0);
        if (sentBytes < 0)
        {
            SPDLOG_ERROR("send() failed");
            closeConn = true;
            break;
        }
    }
    
    // If the closeConn flag was turned on, we need to clean up this active connection. This
    // clean up process includes removing the descriptor.
    if (closeConn)
    {
        close(socketFd);
        socketFd = -1;
    }
    
    return closeConn;
}
