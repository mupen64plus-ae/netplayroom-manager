#include <unistd.h>

#include <iostream>
#include <memory>

#define SPDLOG_ACTIVE_LEVEL 0

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_sinks.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/async.h"

int main() 
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
        
    // Compile time log levels
    // define SPDLOG_ACTIVE_LEVEL to desired level
    SPDLOG_INFO("global output with arg {}", 1); // [source main.cpp] [function main] [line 16] global output with arg 1   
    SPDLOG_DEBUG("global output with arg {}", 1); // [source main.cpp] [function main] [line 16] global output with arg 1   
    
    
    usleep(1000*1000*1);
}

