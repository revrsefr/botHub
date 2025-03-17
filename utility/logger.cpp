#include "logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <iostream>
#include <filesystem>

void initialize_logger() {
    try {
        // ✅ Ensure the "run/logs" directory exists
        std::filesystem::create_directories("run/logs");

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "run/logs/bot.log", 1024 * 1024 * 5, 3  // ✅ Max 5MB per file, keep last 3 logs
        );

        auto logger = std::make_shared<spdlog::logger>("multi_sink", spdlog::sinks_init_list{console_sink, file_sink});
        spdlog::set_default_logger(logger);
        
        spdlog::flush_every(std::chrono::seconds(1));

        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
        spdlog::info("✅ Logger initialized.");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "❌ Log initialization failed: " << ex.what() << std::endl;
    }
}

void check_and_reinitialize_logger() {
    if (!std::filesystem::exists("run/logs/bot.log")) {
        spdlog::warn("⚠️ Log file deleted! Reinitializing logger...");
        initialize_logger();
    }
}
