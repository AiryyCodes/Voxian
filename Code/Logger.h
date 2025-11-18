#pragma once

#include "Memory.h"

#include <spdlog/logger.h>
#include <spdlog/spdlog.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

class Logger
{
public:
    static Ref<spdlog::logger> GetConsoleLogger() { return m_ConsoleLogger; }

private:
    static inline Ref<spdlog::logger> m_ConsoleLogger = spdlog::stdout_color_mt("console");
};

#define LOG_INFO(...) Logger::GetConsoleLogger()->info(__VA_ARGS__);
#define LOG_WARN(...) Logger::GetConsoleLogger()->warn(__VA_ARGS__);
#define LOG_ERROR(...) Logger::GetConsoleLogger()->error(__VA_ARGS__);
#define LOG_DEBUG(...) Logger::GetConsoleLogger()->debug(__VA_ARGS__);
