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

#include <sys/poll.h>

#include <array>
#include <unordered_map>

#include "ClientHandler.hpp"

/**
 * Used to handle message from any client that connects
 */
class TcpSocketHandler
{
public:
    
    /**
     * Constructor
     * @param portNumber Port number to listen in
     */
    TcpSocketHandler(int portNumber);
	
    /**
     * Start listening
     */
    void startServer();
	
private:
    
    /**
     * Accept new connections
     * @param socketFd Socket handle to listen on
     * @return true on success
     */
    bool acceptNewConnections(int socketFd);
    
    /**
     * Process any received data
     * @param socketFd Socket to process data from
     * @return True if socket was closed
     */
    bool processData(int& socketFd);

    // Port number used to listen in
    int mPortNumber;
    
    // True if we want to end the server
    bool mEndServer;
    
    // File descriptors for clients
    std::array<pollfd, 10000> mFds;
    
    // Map of socket handle number to client handlers
    std::unordered_map<int, ClientHandler> mcClients;
    
    // Current number of file descriptors
    int mNumberFileDescriptors;
};