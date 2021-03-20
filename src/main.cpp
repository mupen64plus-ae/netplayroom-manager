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

#include <unistd.h>

#include <iostream>
#include <memory>

#define SPDLOG_ACTIVE_LEVEL 0

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"

#include "RoomManager.hpp"
#include "TcpSocketHandler.hpp"

void setupLogging()
{
    spdlog::init_thread_pool(8192, 1);
    spdlog::set_level(spdlog::level::trace);
    
    // Create a file rotating logger with 5mb size max and 3 rotated files
    auto max_size = 1048576 * 5;
    auto max_files = 10;
    
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    console_sink->set_pattern("%D,%H:%M:%S,%^%l%$,%s:%#,\"%v\"");

    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/np-room-manager.log", max_size, max_files);
    rotating_sink->set_level(spdlog::level::trace);
    rotating_sink->set_pattern("%D,%H:%M:%S,%^%l%$,%s:%#,\"%v\"");
    
    std::vector<spdlog::sink_ptr> sinks {console_sink, rotating_sink};
    auto logger = std::make_shared<spdlog::async_logger>("logger", sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    logger->set_level(spdlog::level::trace);
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
}

int main() 
{
    setupLogging();
    
    SPDLOG_INFO("Netplay room manager started");
    
    RoomManager roomManager;
    
    TcpSocketHandler socketHandler(roomManager, 37520);

    socketHandler.startServer();
}

